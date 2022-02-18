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

#ifndef SCENE_COLUMNS_H
#define SCENE_COLUMNS_H

#include "vulkan_engine.h"

void SceneColumnsInit(vulkan_context_t* vulkan, VkBuffer* dft_storage_buffers);
void SceneColumnsRecreateFramebuffers(vulkan_context_t* vulkan);
void SceneColumnsRender(vulkan_context_t* vulkan, VkCommandBuffer frame_command_buffer, uint32_t frame_image_index, uint32_t frame_resource_index);
void SceneColumnsDestroy(vulkan_context_t* vulkan);

#endif