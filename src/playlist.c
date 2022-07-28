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

#include "playlist.h"

#include <windows.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>

typedef struct linked_list_t linked_list_t;
struct linked_list_t 
{
    song_t* pointer;
    linked_list_t* previous;
    linked_list_t* next;
};

void PlaylistInit(playlist_t* playlist)
{
    assert(playlist != NULL);

    playlist->song_paths = NULL;
    playlist->songs = NULL;
    playlist->songs_shuffled = NULL;
    playlist->song_count = 0;
    playlist->current_song_index = 0;
}

playlist_error_e PlaylistLoad(char* playlist_file_path, playlist_t* playlist)
{
    assert(playlist != NULL);
    assert(playlist->song_paths == NULL);
    assert(playlist->songs == NULL);
    assert(playlist->songs_shuffled == NULL);

    // Local output value
    playlist_t playlist_tmp;
    PlaylistInit(&playlist_tmp);
    
    FILE* playlist_file = fopen(playlist_file_path, "r");
    if (playlist_file == NULL)
    {
        printf("Failed to open file %s\n", playlist_file_path);
        return PLAYLIST_ERROR_UNABLE_TO_OPEN_FILE;
    }

    /** Compute
     *  - how much memory is needed to store all file names that end with '.wav'
     *  - number of songs in playlist
     */
    char line[MAX_PATH];
    uint64_t total_memory_required = 0;
    while (fgets(line, MAX_PATH, playlist_file) != NULL)
    {
        size_t line_length = strlen(line) - 1 + 1; // -1 to truncate new-line, but +1 to include null-terminating
        if ((strncmp(line + line_length - 5, ".wav", 4) == 0) ||
            (strncmp(line + line_length - 6, ".flac", 5) == 0))
        {
            total_memory_required += line_length;
            playlist_tmp.song_count++;
        }
    }
    if (playlist_tmp.song_count == 0)
    {
        return PLAYLIST_ERROR_EMPTY;
    }

    // Allocate memory for playlist
    playlist_tmp.song_paths = (char*)malloc(total_memory_required);
    playlist_tmp.songs = (song_t*)malloc(playlist_tmp.song_count * sizeof(song_t));
    playlist_tmp.songs_shuffled = (song_t*)malloc(playlist_tmp.song_count * sizeof(song_t));

    // Fill playlist song names
    fseek(playlist_file, 0, SEEK_SET);
    uint64_t songs_offset = 0;
    uint64_t current_song_index = 0;
    // TODO (Daniel): avoid reading from the file again?
    while (fgets(line, MAX_PATH, playlist_file) != NULL)
    {
        size_t line_length = strlen(line) - 1 + 1; // -1 to truncate new-line, but +1 to include null-terminating
        song_type_e song_type = SONG_TYPE_INVALID;
        if (strncmp(line + line_length - 5, ".wav", 4) == 0)
        {
            song_type = SONG_TYPE_WAV;
        }
        else if (strncmp(line + line_length - 6, ".flac", 5) == 0)
        {
            song_type = SONG_TYPE_FLAC;
        }

        if (song_type != SONG_TYPE_INVALID)
        {
            SongInit(&playlist_tmp.songs[current_song_index]);
            playlist_tmp.songs[current_song_index].song_path_offset = playlist_tmp.song_paths + songs_offset;
            memcpy(playlist_tmp.songs[current_song_index].song_path_offset, line, line_length);
            playlist_tmp.songs[current_song_index].song_path_offset[line_length - 1] = '\0'; // Change final character in line from \n to \0
            playlist_tmp.songs[current_song_index].song_type = song_type;

            songs_offset += line_length;
            current_song_index++;
        }
    }
    fclose(playlist_file);

    // Write output value
    *playlist = playlist_tmp;

    return PLAYLIST_ERROR_NO;
}

void PlaylistShuffle(playlist_t* playlist)
{
    assert(playlist != NULL);
    assert(playlist->song_paths != NULL);
    assert(playlist->songs != NULL);
    assert(playlist->songs_shuffled != NULL);

    // TODO (Daniel): only call once
    srand(time(NULL));

    // Turn current offsets into a linked-list
    linked_list_t* linked_list = (linked_list_t*)malloc(playlist->song_count * sizeof(linked_list_t));
    for (uint64_t i = 0; i < playlist->song_count; i++)
    {
        linked_list[i].pointer = (song_t*)&playlist->songs[i];
        if (i == 0)
        {
            linked_list[i].previous = NULL;
        }
        else
        {
            linked_list[i].previous = &linked_list[i - 1];
        }
        if (i == (playlist->song_count - 1))
        {
            linked_list[i].next = NULL;
        }
        else
        {
            linked_list[i].next = &linked_list[i + 1];
        }
    }

    // Generate shuffled order
    linked_list_t* linked_list_tmp = linked_list;
    for (uint64_t i = 0; i < playlist->song_count; i++)
    {
        // Pick random song
        uint64_t remaining_song_count = playlist->song_count - i;
        int random_song_index = rand() % remaining_song_count;

        // Assign song to current index in shuffled songs
        playlist->songs_shuffled[i] = *linked_list_tmp[random_song_index].pointer;

        // Remove picked song from linked list
        if (linked_list_tmp[random_song_index].previous == NULL)
        {
            linked_list_tmp = linked_list_tmp[random_song_index].next;
        }
        else if (linked_list_tmp[random_song_index].next == NULL)
        {
            linked_list_tmp[random_song_index].previous->next = NULL;
        }
        else
        {
            linked_list_tmp[random_song_index].previous->next = linked_list_tmp[random_song_index].next;
            linked_list_tmp[random_song_index].next->previous = linked_list_tmp[random_song_index].previous;
        }
    }

    // Clean-up
    free(linked_list);
}

void PlaylistFree(playlist_t* playlist)
{
    assert(playlist != NULL);
    assert(playlist->song_paths != NULL);
    assert(playlist->songs != NULL);
    assert(playlist->songs_shuffled != NULL);

    free(playlist->songs_shuffled);
    free(playlist->songs);
    free(playlist->song_paths);
}

playlist_error_e PlaylistGenerate(const char* directory_path, const char* playlist_output_file_path)
{
    assert(directory_path != NULL);
    assert(playlist_output_file_path != NULL);

    // Append "/*" to directory path if necessary
    size_t directory_path_length_original = strlen(directory_path);
    size_t directory_path_length_final = directory_path_length_original;
    if ((directory_path[directory_path_length_original - 1] != '/') &&
        (directory_path[directory_path_length_original - 1] != '\\'))
    {
        // Add '/', '*' and '\0' to the end
        directory_path_length_final += 3;
    }
    else
    {
        // Add '*' and '\0' to the end
        directory_path_length_final += 2;
    }
    char* directory_path_final = (char*)malloc(directory_path_length_final * sizeof(char));
    strcpy(directory_path_final, directory_path);
    if ((directory_path[directory_path_length_original - 1] != '/') &&
        (directory_path[directory_path_length_original - 1] != '\\'))
    {
        directory_path_final[directory_path_length_original]     = '/';
        directory_path_final[directory_path_length_original + 1] = '*';
        directory_path_final[directory_path_length_original + 2] = '\0';
    }
    else
    {
        directory_path_final[directory_path_length_original]     = '*';
        directory_path_final[directory_path_length_original + 1] = '\0';
    }

    // Open directory
    WIN32_FIND_DATAA dir_data;
    HANDLE dir = FindFirstFileA(directory_path_final, &dir_data);
    if (dir == INVALID_HANDLE_VALUE)
    {
        free(directory_path_final);
        printf("Failed to open directory '%s'\n", directory_path_final);
        return PLAYLIST_ERROR_UNABLE_TO_OPEN_FILE;
    }
    
    // Skip files '.' and '..'
    FindNextFileA(dir, &dir_data);
    FindNextFileA(dir, &dir_data);

    // Open playlist file for writing
    FILE* playlist_file = fopen(playlist_output_file_path, "w");
    if (playlist_file == NULL)
    {
        free(directory_path_final);
        printf("Failed to open file '%s'\n", playlist_output_file_path);
        return PLAYLIST_ERROR_UNABLE_TO_OPEN_FILE;
    }

    // Iterate files in directory
    char current_file_path[MAX_PATH];
    size_t current_file_path_first_char_start_index = directory_path_length_original;
    strcpy(current_file_path, directory_path);
    if ((directory_path[directory_path_length_original - 1] != '/') &&
        (directory_path[directory_path_length_original - 1] != '\\'))
    {
        current_file_path_first_char_start_index += 1;
        current_file_path[directory_path_length_original] = '/';
    }
    printf("Files:\n");
    while (1)
    {
        strcpy(current_file_path + current_file_path_first_char_start_index, dir_data.cFileName);
        fwrite(current_file_path, strlen(current_file_path), 1, playlist_file);
        fwrite("\n", 1, 1, playlist_file);
        printf("\t%s\n", current_file_path);

        if (FindNextFileA(dir, &dir_data) == 0)
        {
            break;
        }
    }

    // Clean-up
    fflush(playlist_file);
    fclose(playlist_file);
    FindClose(dir);
    free(directory_path_final);

    return PLAYLIST_ERROR_NO;
}