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

#ifndef SCENE_UI_H
#define SCENE_UI_H

#include "vulkan_engine.h"

#define INFO_SECTION_ROW_TITLE            0u
#define INFO_SECTION_ROW_LOOP             1u
#define INFO_SECTION_ROW_SHUFFLE          2u
#define INFO_SECTION_ROW_PLAYLIST         3u
#define INFO_SECTION_ROW_SONG             4u
#define INFO_SECTION_ROW_ARTIST           5u
#define INFO_SECTION_ROW_ALBUM            6u
#define INFO_SECTION_ROW_CHANNEL_COUNT    7u
#define INFO_SECTION_ROW_SAMPLE_RATE      8u
#define INFO_SECTION_ROW_BITS_PER_SAMPLE  9u
#define INFO_SECTION_ROW_ERROR           10u
#define INFO_SECTION_ROW_COUNT           11u

void SceneUIInit(vulkan_context_t* vulkan);
void SceneUIRecreateFramebuffers(vulkan_context_t* vulkan);
void SceneUIUpdateInfoMessage(const char* message, uint32_t row);
void SceneUIRender(vulkan_context_t* vulkan, VkCommandBuffer frame_command_buffer, uint32_t frame_image_index, uint32_t frame_resource_index, bool ui_showing, char* sound_player_command_string, uint16_t sound_player_command_string_length);
void SceneUIDestroy(vulkan_context_t* vulkan);

#endif