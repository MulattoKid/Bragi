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

#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/1.3.204/vulkan.h"

#include <stdio.h>
#include <windows.h>

#define VK_CHECK_RES(res) if (res != VK_SUCCESS) { printf("Vulkan call %s:%lu failed\n", __FILE__, __LINE__); exit(EXIT_FAILURE); }
#define VULKAN_MAX_FRAMES_IN_FLIGHT 2

// TODO (Daniel): pack
struct vulkan_context_t
{
    uint32_t target_api_version;
    uint32_t instance_api_version;
    uint32_t physical_device_api_version;

    // Phyiscal device
    VkPhysicalDevice                 physical_device;
    VkPhysicalDeviceProperties       physical_device_props;
    VkPhysicalDeviceMemoryProperties physical_device_memory_props;
    VkFormatProperties*              physical_device_format_props;

    // Surface
    VkSurfaceKHR             surface;
    VkSurfaceCapabilitiesKHR surface_caps;
    VkFormat                 surface_format;
    VkColorSpaceKHR          surface_color_space;
    VkPresentModeKHR         surface_present_mode;

    // Queue
    uint32_t queue_index;
    VkQueue  queue;

    // Device
    VkDevice device;

    // Swapchain
    VkSwapchainKHR swapchain;
    uint32_t       swapchain_image_count;
    VkImage*       swapchain_images;
    VkImageView*   swapchain_image_views;

    // Actual rendering resolution
    float    rendering_scale;
    uint32_t rendering_width;
    uint32_t rendering_height;

    // Depth/Stencil image
    VkFormat       depth_stencil_format;
    VkImage        depth_stencil_image;
    VkDeviceMemory depth_stencil_image_memory;
    VkImageView    depth_stencil_image_view;

    // Intermediate swapchain image
    VkFormat       intermediate_swapchain_image_format;
    VkImage        intermediate_swapchain_image;
    VkDeviceMemory intermediate_swapchain_image_memory;
    VkImageView    intermediate_swapchain_image_view;

    // Command buffers
    VkCommandBuffer* command_buffers;

    // Synchronization objecs
    VkSemaphore* semaphores_image_available;
    VkSemaphore* semaphores_render_finished;
    VkFence*     fences_frame_in_flight;

    // Pipeline Cache
    VkPipelineCacheHeaderVersionOne pipeline_cache_header;
    VkPipelineCache pipeline_cache;

    // Function pointers
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT   vkCmdEndDebugUtilsLabelEXT;

    // For naming objects
    char vulkan_object_name[128];
};

// Base functions
void VulkanInit(HINSTANCE win_instance, HWND win_window, vulkan_context_t* vulkan_context);
void VulkanSetObjectName(VkDevice device, VkObjectType object_type, uint64_t object, const char* name, PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT);
void VulkanSetObjectName(vulkan_context_t* vulkan, VkObjectType object_type, uint64_t object, const char* name);
// Swapchain
void VulkanDestroySwapchain(vulkan_context_t* vulkan);
void VulkanRecreateSwapchain(vulkan_context_t* vulkan);
// Shaders
void VulkanCreateShader(vulkan_context_t* vulkan, const char* file, VkShaderModule* shader, const char* shader_name);
// Buffers
void VulkanCreateBuffer(vulkan_context_t* vulkan, void* buffer_data, VkDeviceSize buffer_data_size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags buffer_memory_properties, VkBuffer* buffer, VkDeviceMemory* buffer_memory, const char* buffer_name, const char* buffer_memory_name);
void VulkanDestroyBuffer(vulkan_context_t* vulkan, VkBuffer* buffer, VkDeviceMemory* buffer_memory);
// Images
void VulkanCreateImage(vulkan_context_t* vulkan, VkImageType image_type, VkFormat image_format, uint32_t image_width, uint32_t image_height, uint32_t image_depth, VkSampleCountFlagBits image_samples, VkImageTiling image_tiling, VkImageUsageFlags image_usage, VkImageViewType image_view_type, VkImageAspectFlags image_aspect, VkImageLayout image_final_layout, void* image_data, VkDeviceSize image_data_size, VkMemoryPropertyFlags image_memory_properties, VkImage* image, VkDeviceMemory *image_memory, VkImageView* image_view, const char* image_name, const char* image_memory_name, const char* image_view_name);
void VulkanCmdTransitionImageLayout(vulkan_context_t* vulkan, VkCommandBuffer command_buffer, VkImage image, VkImageLayout image_old_layout, VkImageLayout image_new_layout, VkImageAspectFlags image_aspect, VkPipelineStageFlags barrier_src_pipeline_stage, VkAccessFlags barrier_src_access_mask, VkPipelineStageFlags barrier_dst_pipeline_stage, VkAccessFlags barrier_dst_access_mask, VkDependencyFlags barrier_dependency);
// Samplers
void VulkanCreateSampler(vulkan_context_t* vulkan, VkFilter sampler_min_filter, VkFilter sampler_mag_filter, VkSampler* sampler, const char* sampler_name);
// Debug Util Labels
void VulkanCmdBeginDebugUtilsLabel(vulkan_context_t* vulkan, VkCommandBuffer command_buffer, const char* debug_label_name);
void VulkanCmdEndDebugUtilsLabel(vulkan_context_t* vulkan, VkCommandBuffer command_buffer);

#endif