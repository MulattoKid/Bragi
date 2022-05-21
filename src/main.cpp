/**
 * Copyright (c) 2022 - 2022, Daniel Fedai Larsen.
 *
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright
 * notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "dft.h"
#include "playlist.h"
#include "scene_columns.h"
#include "scene_ui.h"
#include "sound_player.h"
// https://nothings.org/stb/font/
#include "stb_font/stb_font_consolas_24_usascii.inl"
#include "vulkan_engine.h"
// https://stackoverflow.com/questions/57137351/line-is-not-constexpr-in-msvc
//#include "tracy-0.7.8/Tracy.hpp"
#include "windows_audio.h"
#include "windows_synchronization.h"
#include "windows_thread.h"
#include "windows_window.h"

#include <windows.h>

#include <assert.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global variables
extern bool window_running;
extern bool window_active;
extern window_key_event window_key_events[UINT8_MAX];
extern uint8_t window_key_releases_index;

int main(int argc, char** argv)
{
    // Name main thread
    wchar_t thread_main_name[] = L"bragi_main_thread";
    HRESULT hres = SetThreadDescription(GetCurrentThread(), thread_main_name);
    if (FAILED(hres))
    {
        printf("ERROR(%s:%i): Failed to name thread '%s'\n", __FILE__, __LINE__, thread_main_name);
        exit(EXIT_FAILURE);
    }




    ///////////////////
    // Windows setup //
    ///////////////////
    // Create Win32 window
    HMODULE instance = NULL;
    HWND window = NULL;
    WindowCreate(&instance, &window);
    bool window_key_shift_held = false;




    //////////////////
    // Vulkan setup //
    //////////////////
    // Init Vulkan
    vulkan_context_t vulkan;
    VulkanInit(instance, window, &vulkan);
    uint32_t frame_number = 0;




    //////////////
    // Settings //
    //////////////
    bool ui_command_line_showing = false;
    bool viz_enabled = false;




    //////////////
    // DFT DATA //
    //////////////
    DWORD dft_previous_frame_sample_position = 0;
    DWORD dft_current_frame_sample_position = 0;
    // DFT buffers
    VkBuffer* dft_storage_buffers = (VkBuffer*)malloc(VULKAN_MAX_FRAMES_IN_FLIGHT * sizeof(VkBuffer));
    VkDeviceMemory* dft_storage_buffer_memories = (VkDeviceMemory*)malloc(VULKAN_MAX_FRAMES_IN_FLIGHT * sizeof(VkDeviceMemory));
    for (uint32_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dft_storage_buffers[i] = VK_NULL_HANDLE;
        dft_storage_buffer_memories[i] = VK_NULL_HANDLE;
        // Initialized to 0
        VulkanCreateBuffer(&vulkan, NULL, DFT_FREQUENCY_BAND_COUNT * sizeof(float), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &dft_storage_buffers[i], &dft_storage_buffer_memories[i], NULL, NULL);
        
        memset(vulkan.vulkan_object_name, 0, 128);
        sprintf(vulkan.vulkan_object_name, "DFT Storage Buffer ");
        sprintf(vulkan.vulkan_object_name + 19, "%u", i);
        VulkanSetObjectName(&vulkan, VK_OBJECT_TYPE_BUFFER, (uint64_t)dft_storage_buffers[i], vulkan.vulkan_object_name);
        memset(vulkan.vulkan_object_name, 0, 128);
        sprintf(vulkan.vulkan_object_name, "DFT Storage Buffer Memory ");
        sprintf(vulkan.vulkan_object_name + 26, "%u", i);
        VulkanSetObjectName(&vulkan, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)dft_storage_buffer_memories[i], vulkan.vulkan_object_name);
    }
    HANDLE dft_current_playback_buffer_shared_shared_mutex = CreateMutexA(NULL, FALSE, "CurrentPlaybackBufferMutex");
    uint64_t dft_current_playback_buffer_shared_size = 8192 * 2; // Same as audio_buffer_size * 2 to have room for a sample-rate converted version of the audio data
    byte_t* dft_current_playback_buffer_shared = (byte_t*)malloc(dft_current_playback_buffer_shared_size);
    uint64_t dft_current_playback_buffer_local_size = 0;
    byte_t* dft_current_playback_buffer_local = (byte_t*)malloc(dft_current_playback_buffer_shared_size);


    // Initialize scenes
    SceneColumnsInit(&vulkan, dft_storage_buffers);
    SceneUIInit(&vulkan);

    // Local data used to store shared data to avoid holding the mutex for an extended period of time
    sound_player_operation_e sound_player_ui_next_operation = SOUND_PLAYER_OP_READY;
    sound_player_loop_e sound_player_loop_state = SOUND_PLAYER_LOOP_NO;
    sound_player_shuffle_e sound_player_shuffle_state = SOUND_PLAYER_SHUFFLE_NO;
    bool sound_player_loop_state_changed = false;
    bool sound_player_shuffle_state_changed = false;
    char sound_player_playlist_next_file_path[MAX_PATH];
    char sound_player_playlist_current_file_path[MAX_PATH];
    char sound_player_song_playing[MAX_PATH];
    char sound_player_artist_playing[MAX_PATH];
    char sound_player_album_playing[MAX_PATH];
    char sound_player_song_info[MAX_PATH];
    memset(sound_player_playlist_next_file_path, 0, MAX_PATH);
    memset(sound_player_playlist_current_file_path, 0, MAX_PATH);
    memset(sound_player_song_playing, 0, MAX_PATH);
    memset(sound_player_artist_playing, 0, MAX_PATH);
    memset(sound_player_album_playing, 0, MAX_PATH);
    memset(sound_player_song_info, 0, MAX_PATH);
    uint16_t sound_player_song_sample_rate = 0;
    uint8_t sound_player_song_channel_count = 0;
    uint8_t sound_player_song_bps = 0; // Bytes per sample
    // The largest audio format we support is two-channel 16-bit per sample
    byte* dft_audio_data = (byte*)malloc(DFT_MAX_WINDOWS * DFT_N * 2 * sizeof(int16_t));
    uint32_t audio_data_size = 0;
    int16_t audio_data_bps = 0; // Bytes per sample
    int16_t audio_data_bytes_per_sample_all_channels = 0;




    ////////////////////////
    // Sound player setup //
    ////////////////////////
    // Data
    char sound_player_command_string[MAX_PATH];
    sound_player_command_string[0] = '_';
    sound_player_command_string[1] = '\0';
    uint16_t sound_player_command_string_index = 0;
    uint16_t sound_player_command_string_length = 1; // Including '_'
    // Set up shared data to sound player
    sound_player_shared_data_t sound_player_shared_data;
    sound_player_shared_data.mutex = CreateMutexA(NULL, FALSE, "SharedDataMutex");
    assert(sound_player_shared_data.mutex != NULL);
    sound_player_shared_data.audio_device = NULL;
    sound_player_shared_data.current_playback_buffer_mutex = dft_current_playback_buffer_shared_shared_mutex;
    sound_player_shared_data.current_playback_buffer = dft_current_playback_buffer_shared;
    sound_player_shared_data.current_playback_buffer_size = 0;
    sound_player_shared_data.song = NULL;
    sound_player_shared_data.event = CreateEventA(NULL, FALSE, FALSE, "SharedDataOperationChangedEvent");
    assert(sound_player_shared_data.event != NULL);
    sound_player_shared_data.ui_next_operation = SOUND_PLAYER_OP_READY;
    sound_player_shared_data.loop_state = SOUND_PLAYER_LOOP_NO;
    sound_player_shared_data.shuffle_state = SOUND_PLAYER_SHUFFLE_NO;
    sound_player_shared_data.playlist_current_changed = false;
    sound_player_shared_data.error_message_changed = false;
    memset(sound_player_shared_data.playlist_next_file_path, 0, MAX_PATH);
    memset(sound_player_shared_data.playlist_current_file_path, 0, MAX_PATH);
    memset(sound_player_shared_data.error_message, 0, MAX_PATH);
    // Start sound player thread
    HANDLE sound_player_thread;
    wchar_t thread_sound_player_name[] = L"bragi_sound_thread";
    ThreadCreate(&SoundPlayerThreadProc, &sound_player_shared_data, thread_sound_player_name, &sound_player_thread);




    // Loop
    //  1) Handle window input
    //  2) Wait for a frame-in-flight's resources become available
    //  3) Acquire image index for next frame to be able to build command buffer
    //  4) Simulate frame on CPU
    //  5) Build command buffer using frame-in-flight's resources
    //  6) Submit command buffer (wait for step 3 to actually acquire swapchain image)
    //  7) Present frame's image (wait for step 6 to finish rendering the frame)
    while (1)
    {
        // Reset data
        sound_player_ui_next_operation = SOUND_PLAYER_OP_READY;
        sound_player_loop_state = SOUND_PLAYER_LOOP_NO;
        sound_player_shuffle_state = SOUND_PLAYER_SHUFFLE_NO;
        sound_player_loop_state_changed = false;
        sound_player_shuffle_state_changed = false;
        audio_data_size = 0;
        audio_data_bps = 0;
        audio_data_bytes_per_sample_all_channels = 0;
        dft_current_playback_buffer_local_size = 0;

        // 1)
        // Have a look in the OS message queue, and if there's a message:
        //  a) Retrieve its information, and remove it from the message queue
        //  b) Process the message
        MSG msg;
        while (PeekMessageA(&msg, window, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (window_running == false)
            {
                // Write out pipeline cache before exiting
                size_t pipeline_cache_data_size;
                VK_CHECK_RES(vkGetPipelineCacheData(vulkan.device, vulkan.pipeline_cache, &pipeline_cache_data_size, NULL));
                byte* pipeline_cache_data = (byte*)malloc(pipeline_cache_data_size * sizeof(byte));
                VK_CHECK_RES(vkGetPipelineCacheData(vulkan.device, vulkan.pipeline_cache, &pipeline_cache_data_size, pipeline_cache_data));
                FILE* pipeline_cache_file = fopen("data/pipeline_cache.bin", "wb");
                assert(pipeline_cache_file != NULL);
                fwrite(&vulkan.pipeline_cache_header, sizeof(VkPipelineCacheHeaderVersionOne), 1, pipeline_cache_file);
                fwrite(pipeline_cache_data, pipeline_cache_data_size, 1, pipeline_cache_file);
                fflush(pipeline_cache_file);
                fclose(pipeline_cache_file);

                exit(EXIT_SUCCESS);
            }
        }
        // Process key events
        for (uint8_t i = 0; i < window_key_releases_index; i++)
        {
            WPARAM key = window_key_events[i].key;
            window_key_action action = window_key_events[i].action;

            switch (key)
            {
                case VK_BACK:
                {
                    if ((sound_player_command_string_index - 1) >= 0)
                    {
                        sound_player_command_string_index--;
                        sound_player_command_string_length--;
                        sound_player_command_string[sound_player_command_string_index]     = '_';
                        sound_player_command_string[sound_player_command_string_index + 1] = '\0';
                    }
                } break;

                case VK_RETURN:
                {
                    if (window_key_shift_held == true)
                    {
                        ui_command_line_showing = !ui_command_line_showing;
                    }
                    else
                    {
                        if (ui_command_line_showing == true)
                        {
                            // Clear current error message
                            SceneUIUpdateInfoMessage("", INFO_SECTION_ROW_ERROR);

                            // Parse command
                            *(sound_player_command_string + sound_player_command_string_index) = '\0'; // Null-terminate argument at '_' place
                            char* command = sound_player_command_string;
                            char* argument = strchr(sound_player_command_string, (int)' ');
                            if (argument != NULL)
                            {
                                *argument = '\0'; // Null-terminate command
                                argument += 1; // Skip actual ' ' (now '\0')
                            }
                            printf("%s : \"%s\"\n", command, argument);

                            if (strcmp(command, "play") == 0)
                            {
                                if (argument == NULL)
                                {
                                    SceneUIUpdateInfoMessage("Command 'play' requires argument", INFO_SECTION_ROW_ERROR);
                                    goto reset_sound_player_command;
                                }

                                char* argument_end = NULL;
                                // Check if argument starts with '"'
                                if (argument[0] == '"')
                                {
                                    argument += 1; // Skip '"'
                                    argument_end = strchr(argument, (int)'"');
                                    if (argument_end == NULL)
                                    {
                                        SceneUIUpdateInfoMessage("If a path starts with \" it must also end with \"", INFO_SECTION_ROW_ERROR);
                                        goto reset_sound_player_command;
                                    }
                                    *argument_end = '\0'; // Null-terminate
                                }

                                sound_player_ui_next_operation = SOUND_PLAYER_OP_PLAY;
                                strcpy(sound_player_playlist_next_file_path, argument);
                            }
                            else if (strcmp(command, "next") == 0)
                            {
                                if (argument != NULL)
                                {
                                    SceneUIUpdateInfoMessage("Command 'next' does not take an argument...ignoring", INFO_SECTION_ROW_ERROR);
                                }

                                sound_player_ui_next_operation = SOUND_PLAYER_OP_NEXT;
                            }
                            else if (strcmp(command, "previous") == 0)
                            {
                                if (argument != NULL)
                                {
                                    SceneUIUpdateInfoMessage("Command 'previous' does not take an argument...ignoring", INFO_SECTION_ROW_ERROR);
                                }

                                sound_player_ui_next_operation = SOUND_PLAYER_OP_PREVIOUS;
                            }
                            else if (strcmp(command, "pause") == 0)
                            {
                                if (argument != NULL)
                                {
                                    SceneUIUpdateInfoMessage("Command 'pause' does not take an argument...ignoring", INFO_SECTION_ROW_ERROR);
                                }

                                sound_player_ui_next_operation = SOUND_PLAYER_OP_PAUSE;
                            }
                            else if (strcmp(command, "resume") == 0)
                            {
                                if (argument != NULL)
                                {
                                    SceneUIUpdateInfoMessage("Command 'resume' does not take an argument...ignoring", INFO_SECTION_ROW_ERROR);
                                }

                                sound_player_ui_next_operation = SOUND_PLAYER_OP_RESUME;
                            }
                            else if (strcmp(command, "loop_no") == 0)
                            {
                                sound_player_loop_state = SOUND_PLAYER_LOOP_NO;
                                sound_player_loop_state_changed = true;
                            }
                            else if (strcmp(command, "loop") == 0)
                            {
                                sound_player_loop_state = SOUND_PLAYER_LOOP_PLAYLIST;
                                sound_player_loop_state_changed = true;
                            }
                            else if (strcmp(command, "loop_single") == 0)
                            {
                                sound_player_loop_state = SOUND_PLAYER_LOOP_SINGLE;
                                sound_player_loop_state_changed = true;
                            }
                            else if (strcmp(command, "shuffle_no") == 0)
                            {
                                sound_player_shuffle_state = SOUND_PLAYER_SHUFFLE_NO;
                                sound_player_shuffle_state_changed = true;
                            }
                            else if (strcmp(command, "shuffle") == 0)
                            {
                                sound_player_ui_next_operation = SOUND_PLAYER_OP_SHUFFLE;
                                sound_player_shuffle_state = SOUND_PLAYER_SHUFFLE_RANDOM;
                                sound_player_shuffle_state_changed = true;
                            }
                            else if (strcmp(command, "taskbar_show") == 0)
                            {
                                vkDeviceWaitIdle(vulkan.device);
                                VulkanDestroySwapchain(&vulkan);
                                WindowTaskbarShow(window);
                                VulkanRecreateSwapchain(&vulkan);
                                SceneColumnsRecreateFramebuffers(&vulkan);
                                SceneUIRecreateFramebuffers(&vulkan);
                            }
                            else if (strcmp(command, "taskbar_hide") == 0)
                            {
                                vkDeviceWaitIdle(vulkan.device);
                                VulkanDestroySwapchain(&vulkan);
                                WindowTaskbarHide(window);
                                VulkanRecreateSwapchain(&vulkan);
                                SceneColumnsRecreateFramebuffers(&vulkan);
                                SceneUIRecreateFramebuffers(&vulkan);
                            }
                            else if (strcmp(command, "viz_enable") == 0)
                            {
                                viz_enabled = true;
                            }
                            else if (strcmp(command, "viz_disable") == 0)
                            {
                                viz_enabled = false;
                            }
                            else if (strcmp(command, "generate_playlist") == 0)
                            {
                                if (argument == NULL)
                                {
                                    SceneUIUpdateInfoMessage("Command 'generate_playlist' requires arguments", INFO_SECTION_ROW_ERROR);
                                    goto reset_sound_player_command;
                                }

                                char* argument_end = NULL;
                                char* argument2 = NULL;
                                // Check if argument starts with '"'
                                if (argument[0] == '"')
                                {
                                    argument += 1; // Skip '"'
                                    argument_end = strchr(argument, (int)'"');
                                    if (argument_end == NULL)
                                    {
                                        SceneUIUpdateInfoMessage("If a path starts with \" it must also end with \"", INFO_SECTION_ROW_ERROR);
                                        goto reset_sound_player_command;
                                    }
                                    *argument_end = '\0'; // Null-terminate
                                    argument_end += 1; // Skip '"'
                                    argument2 = strchr(argument_end, (int)' ');
                                }
                                else
                                {
                                    argument2 = strchr(argument, (int)' ');
                                }

                                // Find argument 2
                                if (argument2 == NULL)
                                {
                                    SceneUIUpdateInfoMessage("Command 'generate_playlist' requires 2 arguments", INFO_SECTION_ROW_ERROR);
                                    goto reset_sound_player_command;
                                }
                                *argument2 = '\0'; // Null-terminate
                                argument2 += 1; // Skip ' '

                                // Check if argument2 starts with '"'
                                char* argument2_end = NULL;
                                if (argument2[0] == '"')
                                {
                                    argument2 += 1; // Skip '"'
                                    argument2_end = strchr(argument2, (int)'"');
                                    if (argument2_end == NULL)
                                    {
                                        SceneUIUpdateInfoMessage("If a path starts with \" it must also end with \"", INFO_SECTION_ROW_ERROR);
                                        goto reset_sound_player_command;
                                    }
                                    *argument2_end = '\0'; // Null-terminate
                                }
                                
                                // Generate playlist
                                playlist_error_e playlist_error = PlaylistGenerate(argument, argument2);
                                switch (playlist_error)
                                {
                                    case PLAYLIST_ERROR_NO: {} break;

                                    case PLAYLIST_ERROR_UNABLE_TO_OPEN_FILE:
                                    {
                                        SceneUIUpdateInfoMessage("Unable to open one of the files supplied to PlaylistGenerate", INFO_SECTION_ROW_ERROR);
                                    } break;

                                    default:
                                    {
                                        printf("Invalid error returned from PlaylistGenerate\n");
                                        exit(EXIT_FAILURE);
                                    } break;
                                }
                            }
                            else
                            {
                                SceneUIUpdateInfoMessage("Invalid command...ignoring", INFO_SECTION_ROW_ERROR);
                            }

reset_sound_player_command:
                            sound_player_command_string[0] = '_';
                            sound_player_command_string[1] = '\0';
                            sound_player_command_string_index = 0;
                            sound_player_command_string_length = 1;
                        }
                    }
                } break;

                case VK_SHIFT:
                {
                    if (action == WINDOW_KEY_ACTION_PRESSED)
                    {
                        window_key_shift_held = true;
                    }
                    else
                    {
                        window_key_shift_held = false;
                    }
                } break;

                default:
                {
                    if (ui_command_line_showing == true)
                    {
                        // Within ASCII character range [SPACE, ~]
                        if ((sound_player_command_string_index < (MAX_PATH - 2)) &&
                            (key >= ' ') &&
                            (key <= '~'))
                        {
                            sound_player_command_string[sound_player_command_string_index]     = (char)key;
                            sound_player_command_string[sound_player_command_string_index + 1] = '_';
                            sound_player_command_string[sound_player_command_string_index + 2] = '\0';
                            sound_player_command_string_index++;
                            sound_player_command_string_length++;
                        }
                        else
                        {
                            assert(0);
                        }
                    }
                } break;
            }
        }
        window_key_releases_index = 0;
        // If the window isn't active, sleep 10ms and try again
        if (window_active == false)
        {
            Sleep(10);
            continue;
        }

        // 2)
        uint32_t frame_resource_index = frame_number % VULKAN_MAX_FRAMES_IN_FLIGHT;
        VK_CHECK_RES(vkWaitForFences(vulkan.device, 1, &vulkan.fences_frame_in_flight[frame_resource_index], VK_TRUE, UINT64_MAX));

        // 3)
        uint32_t frame_image_index;
        VK_CHECK_RES(vkAcquireNextImageKHR(vulkan.device, vulkan.swapchain, UINT64_MAX, vulkan.semaphores_image_available[frame_resource_index], VK_NULL_HANDLE, &frame_image_index));
        VK_CHECK_RES(vkResetFences(vulkan.device, 1, &vulkan.fences_frame_in_flight[frame_resource_index]));

        // 4)
        // Update shared data
        // This section is tried minimized in the time it will hold the mutex. Some data will be copied
        // from the shared data over to local memory to avoid holding the mutex while processing said data.
        // This is also the only place where the mutex will be locked.
        SyncLockMutex(sound_player_shared_data.mutex, INFINITE, __FILE__, __LINE__);
        // Update current playlist
        if (sound_player_shared_data.playlist_current_changed == true)
        {
            strcpy(sound_player_playlist_current_file_path, sound_player_shared_data.playlist_current_file_path);
            sound_player_shared_data.playlist_current_changed = false;
        }
        if (sound_player_shared_data.song != NULL)
        {
            strcpy(sound_player_song_playing, sound_player_shared_data.song->title);
            strcpy(sound_player_artist_playing, sound_player_shared_data.song->artist);
            strcpy(sound_player_album_playing, sound_player_shared_data.song->album);
            sound_player_song_channel_count = sound_player_shared_data.song->channel_count;
            sound_player_song_sample_rate = sound_player_shared_data.song->sample_rate;
            sound_player_song_bps = sound_player_shared_data.song->bps;
        }

        // Store string for error message if changed from sound player
        if (sound_player_shared_data.error_message_changed == true)
        {
            SceneUIUpdateInfoMessage(sound_player_shared_data.error_message, INFO_SECTION_ROW_ERROR);
            sound_player_shared_data.error_message_changed = false;
        }
        // Update loop-state in sound player
        if (sound_player_loop_state_changed == true)
        {
            sound_player_shared_data.loop_state = sound_player_loop_state;
        }
        // Update shuffle state in sound player
        if (sound_player_shuffle_state_changed == true)
        {
            sound_player_shared_data.shuffle_state = sound_player_shuffle_state;
        }
        // Update next operation in sound player
        if (sound_player_ui_next_operation != SOUND_PLAYER_OP_READY)
        {
            sound_player_shared_data.ui_next_operation = sound_player_ui_next_operation;
            switch (sound_player_ui_next_operation)
            {
                case SOUND_PLAYER_OP_PLAY:
                {
                    strcpy(sound_player_shared_data.playlist_next_file_path, sound_player_playlist_next_file_path);
                } break;

                default: {} break;
            }
            SyncSetEvent(sound_player_shared_data.event, __FILE__, __LINE__);
        }
        SyncReleaseMutex(sound_player_shared_data.mutex, __FILE__, __LINE__);

        // Get and store samples to be used for DFT from sound player
        if ((viz_enabled == true) &&
            (sound_player_shared_data.audio_device != NULL))
        {
            DWORD mutex_locked = SyncTryLockMutex(dft_current_playback_buffer_shared_shared_mutex, 0, __FILE__, __LINE__);
            if (mutex_locked == WAIT_OBJECT_0)
            {
                assert(dft_current_playback_buffer_shared_size >= sound_player_shared_data.current_playback_buffer_size); // Just check that we have enough space
                dft_current_playback_buffer_local_size = sound_player_shared_data.current_playback_buffer_size;
                memcpy(dft_current_playback_buffer_local, dft_current_playback_buffer_shared, dft_current_playback_buffer_local_size);
                SyncReleaseMutex(dft_current_playback_buffer_shared_shared_mutex, __FILE__, __LINE__);
            }
        }
        // Potentially compute DFT
        if ((viz_enabled == true) &&
            (dft_current_playback_buffer_local_size > 0))
        {
            float* dft_bands = NULL;
            VK_CHECK_RES(vkMapMemory(vulkan.device, dft_storage_buffer_memories[frame_resource_index], 0, VK_WHOLE_SIZE, 0, (void**)&dft_bands));
            DFTCompute(dft_current_playback_buffer_local, dft_current_playback_buffer_local_size / sound_player_shared_data.song->bps / sound_player_shared_data.song->channel_count, sound_player_shared_data.song->bps, sound_player_shared_data.song->channel_count * sound_player_shared_data.song->bps, dft_bands);
            vkUnmapMemory(vulkan.device, dft_storage_buffer_memories[frame_resource_index]);
        }

        // Update UI strings
        if (sound_player_loop_state_changed == true)
        {
            switch (sound_player_loop_state)
            {
                case SOUND_PLAYER_LOOP_NO:
                {
                    SceneUIUpdateInfoMessage("no loop", INFO_SECTION_ROW_LOOP);
                } break;

                case SOUND_PLAYER_LOOP_PLAYLIST:
                {
                    SceneUIUpdateInfoMessage("loop playlist", INFO_SECTION_ROW_LOOP);
                } break;

                case SOUND_PLAYER_LOOP_SINGLE:
                {
                    SceneUIUpdateInfoMessage("loop single", INFO_SECTION_ROW_LOOP);
                } break;
            }
        }
        if (sound_player_shuffle_state_changed == true)
        {
            switch (sound_player_shuffle_state)
            {
                case SOUND_PLAYER_SHUFFLE_NO:
                {
                    SceneUIUpdateInfoMessage("no shuffle", INFO_SECTION_ROW_SHUFFLE);
                } break;

                case SOUND_PLAYER_SHUFFLE_RANDOM:
                {
                    SceneUIUpdateInfoMessage("shuffle", INFO_SECTION_ROW_SHUFFLE);
                }
            }
        }
        // TODO (Daniel): playlist name is updated in info section even if loading of playlist fails
        SceneUIUpdateInfoMessage(sound_player_playlist_current_file_path, INFO_SECTION_ROW_PLAYLIST);
        SceneUIUpdateInfoMessage(sound_player_song_playing, INFO_SECTION_ROW_SONG);
        SceneUIUpdateInfoMessage(sound_player_artist_playing, INFO_SECTION_ROW_ARTIST);
        SceneUIUpdateInfoMessage(sound_player_album_playing, INFO_SECTION_ROW_ALBUM);
        SceneUIUpdateInfoMessage(_itoa(sound_player_song_channel_count, sound_player_song_info, 10), INFO_SECTION_ROW_CHANNEL_COUNT);
        SceneUIUpdateInfoMessage(_itoa(sound_player_song_sample_rate, sound_player_song_info, 10), INFO_SECTION_ROW_SAMPLE_RATE);
        SceneUIUpdateInfoMessage(_itoa(sound_player_song_bps * 8, sound_player_song_info, 10), INFO_SECTION_ROW_BITS_PER_SAMPLE);

        // 5)
        // Begin
        VkCommandBuffer frame_command_buffer = vulkan.command_buffers[frame_resource_index];
        VkCommandBufferBeginInfo frame_command_buffer_begin_info;
        frame_command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        frame_command_buffer_begin_info.pNext = NULL;
        frame_command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        frame_command_buffer_begin_info.pInheritanceInfo = NULL;
        vkBeginCommandBuffer(frame_command_buffer, &frame_command_buffer_begin_info);

        // We only have a singel depth-stencil and intermediate swapchain image, even though there are multiple
        // swapchain images, and multiple frames can be in-flight at the same time. We therefore need to synchronize
        // the use of these two resources so that frame N is finished using them before frame N+1 starts using them.
        VkMemoryBarrier pre_frame_barrier;
        pre_frame_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        pre_frame_barrier.pNext = NULL;
        // Depth-stencil image: frame N must be finished with EZS+LZS, and written data made available
        // before frame N+1 can start reading and writing during EZS+LZS
        VkPipelineStageFlags srcStage   = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        pre_frame_barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        VkPipelineStageFlags dstStage   = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        pre_frame_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        // Intermediate swapchain image: the last step of a frame is blitting the intermediate swapchain image to the
        // swapchain image (a transfer operation). Frame N must have finished this operation, and written data made
        // available before frame N+1 can start reading and writing during color output
        srcStage                        |= VK_PIPELINE_STAGE_TRANSFER_BIT;
        pre_frame_barrier.srcAccessMask |= VK_ACCESS_TRANSFER_WRITE_BIT;
        dstStage                        |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        pre_frame_barrier.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        vkCmdPipelineBarrier(frame_command_buffer, srcStage, dstStage, 0, 1, &pre_frame_barrier, 0, NULL, 0, NULL);

        // Transition intermediate swapchain image from TRANSFER_SRC to COLOR_ATTACHMENT
        VulkanCmdTransitionImageLayout(&vulkan, frame_command_buffer, vulkan.intermediate_swapchain_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT , 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);

        // Render scene
        // Requirements:
        //  1) The intermediate swapchain image is ready to be written to from the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage
        //  2) The intermediate swapchain image is in layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        //  3) The intermediate swapchain image will be in layout VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL afterwards
        //  4) The depth/stencil image is ready to be used in VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT stages
        //  5) The depth/stencil image is in layout VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        //  6) The depth/stencil image will be in layout VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL afterwards
        //  7) The function is responsible for ensuring all other synchronization
        //    a) Any other barriers regarding the intermediate swapchain or depth/stencil image
        //    b) Using the correct resources for the current frame (framebuffer corresponding to frame_image_index, and resources corresponding to frame_resource_index)
        if (viz_enabled == true)
        {
            SceneColumnsRender(&vulkan, frame_command_buffer, frame_image_index, frame_resource_index);
        }

        // Ensure color has been written out before writing color in the UI render pass
        VkMemoryBarrier ui_frame_barrier;
        ui_frame_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        ui_frame_barrier.pNext = NULL;
        ui_frame_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        ui_frame_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        vkCmdPipelineBarrier(frame_command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 1, &ui_frame_barrier, 0, NULL, 0, NULL);

        // Render UI
        SceneUIRender(&vulkan, frame_command_buffer, frame_image_index, frame_resource_index, ui_command_line_showing, sound_player_command_string, sound_player_command_string_length);

        // Barrier
        //VulkanCmdBeginDebugUtilsLabel(&vulkan, frame_command_buffer, "Blit Intermediate Swapchain Image to Swapchain Image");
        // Transfer intermediate swapchain image from COLOR_ATTACHMENT to TRANSFER_SRC
        VulkanCmdTransitionImageLayout(&vulkan, frame_command_buffer, vulkan.intermediate_swapchain_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);
        // Transfer swapchain image from PRESENT_SRC to TRANSFER_DST
        VulkanCmdTransitionImageLayout(&vulkan, frame_command_buffer, vulkan.swapchain_images[frame_image_index], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT, 0);

        // Blit intermediate output to swapchain
        VkImageBlit blit_region;
        blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit_region.srcSubresource.mipLevel = 0;
        blit_region.srcSubresource.baseArrayLayer = 0;
        blit_region.srcSubresource.layerCount = 1;
        blit_region.srcOffsets[0] = { 0, 0, 0};
        blit_region.srcOffsets[1] = { (int32_t)vulkan.surface_caps.currentExtent.width, (int32_t)vulkan.surface_caps.currentExtent.height, 1 };
        blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit_region.dstSubresource.mipLevel = 0;
        blit_region.dstSubresource.baseArrayLayer = 0;
        blit_region.dstSubresource.layerCount = 1;
        blit_region.dstOffsets[0] = { 0, 0, 0};
        blit_region.dstOffsets[1] = { (int32_t)vulkan.surface_caps.currentExtent.width, (int32_t)vulkan.surface_caps.currentExtent.height, 1 };
        vkCmdBlitImage(frame_command_buffer, vulkan.intermediate_swapchain_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vulkan.swapchain_images[frame_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region, VK_FILTER_NEAREST);

        // Barrier
        VulkanCmdTransitionImageLayout(&vulkan, frame_command_buffer, vulkan.swapchain_images[frame_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0);
        //VulkanCmdEndDebugUtilsLabel(&vulkan, frame_command_buffer);

        // End
        vkEndCommandBuffer(frame_command_buffer);

        // 6)
        VkSubmitInfo frame_submit_info;
        frame_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        frame_submit_info.pNext = NULL;
        frame_submit_info.waitSemaphoreCount = 1;
        frame_submit_info.pWaitSemaphores = &vulkan.semaphores_image_available[frame_resource_index];
        // Cannot do color output before the swapchain image is available
        VkPipelineStageFlags frame_submit_info_wait_stages = (VkPipelineStageFlags)(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        frame_submit_info.pWaitDstStageMask = &frame_submit_info_wait_stages;
        frame_submit_info.commandBufferCount = 1;
        frame_submit_info.pCommandBuffers = &frame_command_buffer;
        frame_submit_info.signalSemaphoreCount = 1;
        frame_submit_info.pSignalSemaphores = &vulkan.semaphores_render_finished[frame_resource_index];
        VK_CHECK_RES(vkQueueSubmit(vulkan.queue, 1, &frame_submit_info, vulkan.fences_frame_in_flight[frame_resource_index]));

        // 7)
        VkPresentInfoKHR frame_present_info;
        frame_present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        frame_present_info.pNext = NULL;
        frame_present_info.waitSemaphoreCount = 1;
        frame_present_info.pWaitSemaphores = &vulkan.semaphores_render_finished[frame_resource_index];
        frame_present_info.swapchainCount = 1;
        frame_present_info.pSwapchains = &vulkan.swapchain;
        frame_present_info.pImageIndices = &frame_image_index;
        frame_present_info.pResults = NULL;
        VK_CHECK_RES(vkQueuePresentKHR(vulkan.queue, &frame_present_info));
        
        frame_number++;
    }

    return EXIT_SUCCESS;
}