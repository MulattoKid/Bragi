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

#include "scene_ui.h"
// https://nothings.org/stb/font/
#include "stb_font/stb_font_consolas_24_usascii.inl"

#include <assert.h>

// Info Section
static float info_section_row_height_pix;
static float info_section_row_height_ndc;
static float info_section_width_pix;
static float info_section_height_pix;
static float info_section_width_ndc;
static float info_section_height_ndc;
static float info_section_top_left_x;
static float info_section_top_left_y;
static float info_section_font_scale;
static VkBuffer info_section_background_vertex_buffer;
static VkDeviceMemory info_section_background_vertex_buffer_memory;
static float info_section_background_color[4] = { 0.4f, 0.0f, 0.0f, 0.75f };
static VkBuffer* info_section_text_vertex_buffers;
static VkDeviceMemory* info_section_text_vertex_buffer_memories;
static float info_section_text_color[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
static char* info_section_texts;
static char** info_section_texts_rows;
static size_t* info_section_texts_row_string_lengths;
static size_t* info_section_texts_row_string_length_to_render;

// Command line
static float command_line_width_pix;
static float command_line_height_pix;
static float command_line_width_ndc;
static float command_line_height_ndc;
static float command_line_top_left_x;
static float command_line_top_left_y;
static float command_line_font_scale;
static VkBuffer command_line_background_vertex_buffer;
static VkDeviceMemory command_line_background_vertex_buffer_memory;
static float command_line_background_color[4] = { 0.4f, 0.0f, 0.0f, 0.75f };
static VkBuffer* command_line_text_vertex_buffers;
static VkDeviceMemory* command_line_text_vertex_buffer_memories;
static float command_line_text_color[4] = { 1.0f, 1.0f, 1.0f, 0.0f };

// Font Resources
stb_fontchar font_character_data[STB_SOMEFONT_NUM_CHARS];
static VkImage font_image;
static VkDeviceMemory font_image_memory;
static VkImageView font_image_view;
static VkSampler font_image_sampler;
static float font_size;

// Descriptor Pools
static VkDescriptorPool descriptor_pool;

// Descriptor Set Layouts
static VkDescriptorSetLayout font_image_sampler_descriptor_set_layout;

// Descriptor Sets
static VkDescriptorSet font_image_sampler_descriptor_set;

// Shaders
static VkShaderModule command_line_background_vertex_shader;
static VkShaderModule command_line_background_fragment_shader;
static VkShaderModule command_line_text_vertex_shader;
static VkShaderModule command_line_text_fragment_shader;

// Graphics Pipeline Layouts
static VkPipelineLayout command_line_background_graphics_pipeline_layout;
static VkPipelineLayout command_line_text_graphics_pipeline_layout;

// Graphics Pipelines
static VkPipeline command_line_background_graphics_pipeline;
static VkPipeline command_line_text_graphics_pipeline;

// Viewport resolution
static float resolution[2];

void SceneUIInit(vulkan_context_t* vulkan)
{
    // Font
    // https://github.com/SaschaWillems/Vulkan/blob/master/examples/textoverlay/textoverlay.cpp
	unsigned char font_pixels[STB_SOMEFONT_BITMAP_HEIGHT][STB_SOMEFONT_BITMAP_WIDTH];
    STB_SOMEFONT_CREATE(font_character_data, font_pixels, STB_SOMEFONT_BITMAP_HEIGHT);
    VulkanCreateImage(vulkan, VK_IMAGE_TYPE_2D, VK_FORMAT_R8_UNORM, (uint32_t)STB_SOMEFONT_BITMAP_WIDTH, (uint32_t)STB_SOMEFONT_BITMAP_HEIGHT, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, (void*)font_pixels, (VkDeviceSize)(STB_SOMEFONT_BITMAP_HEIGHT * STB_SOMEFONT_BITMAP_WIDTH), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &font_image, &font_image_memory, &font_image_view, "Font Image", "Font Image Memory", "Font Image View");
    VulkanCreateSampler(vulkan, VK_FILTER_LINEAR, VK_FILTER_LINEAR, &font_image_sampler, "Font Image Sampler");
    font_size = 24.0f;

    // Info section background vertex buffer
    info_section_row_height_pix = (float)vulkan->surface_caps.currentExtent.height * 0.01f;
    info_section_row_height_ndc = (info_section_row_height_pix / (float)vulkan->surface_caps.currentExtent.height) * 2.0f;
    info_section_width_pix = (float)vulkan->surface_caps.currentExtent.width * 0.2f;
    info_section_height_pix = (float)INFO_SECTION_ROW_COUNT * info_section_row_height_pix;
    info_section_width_ndc = (info_section_width_pix / (float)vulkan->surface_caps.currentExtent.width) * 2.0f;
    info_section_height_ndc = (info_section_height_pix / (float)vulkan->surface_caps.currentExtent.height) * 2.0f;
    info_section_top_left_x = -1.0f;
    info_section_top_left_y = -1.0f;
    info_section_font_scale = info_section_row_height_pix / font_size;
    float info_section_background_vertex_buffer_data[] = {
        info_section_top_left_x,                          info_section_top_left_y,                           // Top left
        info_section_top_left_x,                          info_section_top_left_y + info_section_height_ndc, // Bottom left
        info_section_top_left_x + info_section_width_ndc, info_section_top_left_y + info_section_height_ndc, // Bottom right
        
        info_section_top_left_x + info_section_width_ndc, info_section_top_left_y + info_section_height_ndc, // Bottom right
        info_section_top_left_x + info_section_width_ndc, info_section_top_left_y,                           // Top right
        info_section_top_left_x,                          info_section_top_left_y                            // Top left
    };
    VulkanCreateBuffer(vulkan, info_section_background_vertex_buffer_data, 6 * 2 * sizeof(float), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &info_section_background_vertex_buffer, &info_section_background_vertex_buffer_memory, "Info Section Background Vertex Buffer", "Command Line Background Vertex Buffer Memory");

    // Info section text vertex buffer
    info_section_text_vertex_buffers = (VkBuffer*)malloc(VULKAN_MAX_FRAMES_IN_FLIGHT * sizeof(VkBuffer));
    info_section_text_vertex_buffer_memories = (VkDeviceMemory*)malloc(VULKAN_MAX_FRAMES_IN_FLIGHT * sizeof(VkDeviceMemory));
    for (uint32_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        info_section_text_vertex_buffers[i] = VK_NULL_HANDLE;
        info_section_text_vertex_buffer_memories[i] = VK_NULL_HANDLE;
        VulkanCreateBuffer(vulkan, NULL, INFO_SECTION_ROW_COUNT * MAX_PATH * 6 * 4 * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &info_section_text_vertex_buffers[i], &info_section_text_vertex_buffer_memories[i], "Info Section Text Vertex Buffer", "Info Section Text Vertex Buffer Memory");
        
        memset(vulkan->vulkan_object_name, 0, 128);
        sprintf(vulkan->vulkan_object_name, "Info Section Text Vertex Buffer ");
        sprintf(vulkan->vulkan_object_name + 32, "%u", i);
        VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_BUFFER, (uint64_t)info_section_text_vertex_buffers[i], vulkan->vulkan_object_name);
        memset(vulkan->vulkan_object_name, 0, 128);
        sprintf(vulkan->vulkan_object_name, "Info Section Text Vertex Buffer Memory ");
        sprintf(vulkan->vulkan_object_name + 39, "%u", i);
        VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)info_section_text_vertex_buffer_memories[i], vulkan->vulkan_object_name);
    }

    // Info section texts
    info_section_texts = (char*)malloc(INFO_SECTION_ROW_COUNT * MAX_PATH * sizeof(char)); // Enough to hold strings for all rows
    info_section_texts_rows = (char**)malloc(INFO_SECTION_ROW_COUNT * sizeof(char*));
    info_section_texts_row_string_lengths = (size_t*)malloc(INFO_SECTION_ROW_COUNT * sizeof(size_t));
    info_section_texts_row_string_length_to_render = (size_t*)malloc(INFO_SECTION_ROW_COUNT * sizeof(size_t));
    for (uint8_t i = 0; i < INFO_SECTION_ROW_COUNT; i++)
    {
        info_section_texts_rows[i] = &info_section_texts[i * MAX_PATH];
    }
    strcpy(info_section_texts_rows[INFO_SECTION_ROW_TITLE], "Info:");
    info_section_texts_row_string_lengths[INFO_SECTION_ROW_TITLE] = strlen(info_section_texts_rows[INFO_SECTION_ROW_TITLE]);
    strcpy(info_section_texts_rows[INFO_SECTION_ROW_LOOP], " Loop mode: no looping");
    info_section_texts_row_string_lengths[INFO_SECTION_ROW_LOOP] = strlen(info_section_texts_rows[INFO_SECTION_ROW_LOOP]);
    strcpy(info_section_texts_rows[INFO_SECTION_ROW_SHUFFLE], " Shuffle mode: no shuffle");
    info_section_texts_row_string_lengths[INFO_SECTION_ROW_SHUFFLE] = strlen(info_section_texts_rows[INFO_SECTION_ROW_SHUFFLE]);
    strcpy(info_section_texts_rows[INFO_SECTION_ROW_PLAYLIST], " Playlist: ");
    info_section_texts_row_string_lengths[INFO_SECTION_ROW_PLAYLIST] = strlen(info_section_texts_rows[INFO_SECTION_ROW_PLAYLIST]);
    strcpy(info_section_texts_rows[INFO_SECTION_ROW_SONG], " Song: ");
    info_section_texts_row_string_lengths[INFO_SECTION_ROW_SONG] = strlen(info_section_texts_rows[INFO_SECTION_ROW_SONG]);
    strcpy(info_section_texts_rows[INFO_SECTION_ROW_ARTIST], " Artist: ");
    info_section_texts_row_string_lengths[INFO_SECTION_ROW_ARTIST] = strlen(info_section_texts_rows[INFO_SECTION_ROW_ARTIST]);
    strcpy(info_section_texts_rows[INFO_SECTION_ROW_ALBUM], " Album: ");
    info_section_texts_row_string_lengths[INFO_SECTION_ROW_ALBUM] = strlen(info_section_texts_rows[INFO_SECTION_ROW_ALBUM]);
    strcpy(info_section_texts_rows[INFO_SECTION_ROW_CHANNEL_COUNT], " Channel count: ");
    info_section_texts_row_string_lengths[INFO_SECTION_ROW_CHANNEL_COUNT] = strlen(info_section_texts_rows[INFO_SECTION_ROW_CHANNEL_COUNT]);
    strcpy(info_section_texts_rows[INFO_SECTION_ROW_SAMPLE_RATE], " Sample rate: ");
    info_section_texts_row_string_lengths[INFO_SECTION_ROW_SAMPLE_RATE] = strlen(info_section_texts_rows[INFO_SECTION_ROW_SAMPLE_RATE]);
    strcpy(info_section_texts_rows[INFO_SECTION_ROW_BITS_PER_SAMPLE], " Bits per sample: ");
    info_section_texts_row_string_lengths[INFO_SECTION_ROW_BITS_PER_SAMPLE] = strlen(info_section_texts_rows[INFO_SECTION_ROW_BITS_PER_SAMPLE]);
    strcpy(info_section_texts_rows[INFO_SECTION_ROW_ERROR], " Error: ");
    info_section_texts_row_string_lengths[INFO_SECTION_ROW_ERROR] = strlen(info_section_texts_rows[INFO_SECTION_ROW_ERROR]);

    // Command line background vertex buffer
    command_line_width_pix = info_section_width_pix;
    command_line_height_pix = info_section_row_height_pix;
    command_line_width_ndc = (command_line_width_pix / (float)vulkan->surface_caps.currentExtent.width) * 2.0f;
    command_line_height_ndc = (command_line_height_pix / (float)vulkan->surface_caps.currentExtent.height) * 2.0f;
    command_line_top_left_x = -1.0f;
    command_line_top_left_y = info_section_top_left_y + info_section_height_ndc; // Start right below info section
    command_line_font_scale = command_line_height_pix / font_size;
    float command_line_background_vertex_buffer_data[] = {
        command_line_top_left_x,                          command_line_top_left_y,                           // Top left
        command_line_top_left_x,                          command_line_top_left_y + command_line_height_ndc, // Bottom left
        command_line_top_left_x + command_line_width_ndc, command_line_top_left_y + command_line_height_ndc, // Bottom right
        
        command_line_top_left_x + command_line_width_ndc, command_line_top_left_y + command_line_height_ndc, // Bottom right
        command_line_top_left_x + command_line_width_ndc, command_line_top_left_y,                           // Top right
        command_line_top_left_x,                          command_line_top_left_y                            // Top left
    };
    VulkanCreateBuffer(vulkan, command_line_background_vertex_buffer_data, 6 * 2 * sizeof(float), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &command_line_background_vertex_buffer, &command_line_background_vertex_buffer_memory, "Command Line Background Vertex Buffer", "Command Line Background Vertex Buffer Memory");

    // Command line text vertex buffer
    command_line_text_vertex_buffers = (VkBuffer*)malloc(VULKAN_MAX_FRAMES_IN_FLIGHT * sizeof(VkBuffer));
    command_line_text_vertex_buffer_memories = (VkDeviceMemory*)malloc(VULKAN_MAX_FRAMES_IN_FLIGHT * sizeof(VkDeviceMemory));
    for (uint32_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        command_line_text_vertex_buffers[i] = VK_NULL_HANDLE;
        command_line_text_vertex_buffer_memories[i] = VK_NULL_HANDLE;
        VulkanCreateBuffer(vulkan, NULL, MAX_PATH * 6 * 4 * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &command_line_text_vertex_buffers[i], &command_line_text_vertex_buffer_memories[i], "Command Line Text Vertex Buffer", "Command Line Text Vertex Buffer Memory");
        
        memset(vulkan->vulkan_object_name, 0, 128);
        sprintf(vulkan->vulkan_object_name, "Command Line Text Vertex Buffer ");
        sprintf(vulkan->vulkan_object_name + 32, "%u", i);
        VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_BUFFER, (uint64_t)info_section_text_vertex_buffers[i], vulkan->vulkan_object_name);
        memset(vulkan->vulkan_object_name, 0, 128);
        sprintf(vulkan->vulkan_object_name, "Command Line Text Vertex Buffer Memory ");
        sprintf(vulkan->vulkan_object_name + 39, "%u", i);
        VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)info_section_text_vertex_buffer_memories[i], vulkan->vulkan_object_name);
    }

    // Render pass
    VkPipelineRenderingCreateInfo pipeline_rendering_info;
    pipeline_rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipeline_rendering_info.pNext = NULL;
    pipeline_rendering_info.viewMask = 0;
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &vulkan->intermediate_swapchain_image_format;
    pipeline_rendering_info.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    pipeline_rendering_info.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    // Descriptor pool
    VkDescriptorPoolSize descriptor_pool_size;
    // Font image+sampler
    descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_pool_size.descriptorCount = 1;
    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = NULL;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = descriptor_pool_size.descriptorCount;
    descriptor_pool_info.poolSizeCount = 1;
    descriptor_pool_info.pPoolSizes = &descriptor_pool_size;
    VK_CHECK_RES(vkCreateDescriptorPool(vulkan->device, &descriptor_pool_info, NULL, &descriptor_pool));

    // Font image sampler descriptor set
    VkDescriptorSetLayoutBinding font_image_sampler_descriptor_set_layout_binding;
    font_image_sampler_descriptor_set_layout_binding.binding = 0;
    font_image_sampler_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    font_image_sampler_descriptor_set_layout_binding.descriptorCount = 1;
    font_image_sampler_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    font_image_sampler_descriptor_set_layout_binding.pImmutableSamplers = NULL;
    VkDescriptorSetLayoutCreateInfo font_image_sampler_descriptor_set_layout_info;
    font_image_sampler_descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    font_image_sampler_descriptor_set_layout_info.pNext = NULL;
    font_image_sampler_descriptor_set_layout_info.flags = 0;
    font_image_sampler_descriptor_set_layout_info.bindingCount = 1;
    font_image_sampler_descriptor_set_layout_info.pBindings = &font_image_sampler_descriptor_set_layout_binding;
    VK_CHECK_RES(vkCreateDescriptorSetLayout(vulkan->device, &font_image_sampler_descriptor_set_layout_info, NULL, &font_image_sampler_descriptor_set_layout));
    VkDescriptorSetAllocateInfo font_image_sampler_descriptor_set_info;
    font_image_sampler_descriptor_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    font_image_sampler_descriptor_set_info.pNext = NULL;
    font_image_sampler_descriptor_set_info.descriptorPool = descriptor_pool;
    font_image_sampler_descriptor_set_info.descriptorSetCount = 1;
    font_image_sampler_descriptor_set_info.pSetLayouts = &font_image_sampler_descriptor_set_layout;
    VK_CHECK_RES(vkAllocateDescriptorSets(vulkan->device, &font_image_sampler_descriptor_set_info, &font_image_sampler_descriptor_set));
    VkDescriptorImageInfo font_image_sampler_descriptor_info;
    font_image_sampler_descriptor_info.sampler = font_image_sampler;
    font_image_sampler_descriptor_info.imageView = font_image_view;
    font_image_sampler_descriptor_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet font_image_sampler_descriptor_set_write;
    font_image_sampler_descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    font_image_sampler_descriptor_set_write.pNext = NULL;
    font_image_sampler_descriptor_set_write.dstSet = font_image_sampler_descriptor_set;
    font_image_sampler_descriptor_set_write.dstBinding = 0;
    font_image_sampler_descriptor_set_write.dstArrayElement = 0;
    font_image_sampler_descriptor_set_write.descriptorCount = 1;
    font_image_sampler_descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    font_image_sampler_descriptor_set_write.pImageInfo = &font_image_sampler_descriptor_info;
    font_image_sampler_descriptor_set_write.pBufferInfo = NULL;
    font_image_sampler_descriptor_set_write.pTexelBufferView = NULL;
    vkUpdateDescriptorSets(vulkan->device, 1, &font_image_sampler_descriptor_set_write, 0, NULL);

    // Command line background graphics pipeline
    VulkanCreateShader(vulkan, "data/shaders/command_line_background.vert.spv", &command_line_background_vertex_shader, "Command Line Background Vertex Shader");
    VulkanCreateShader(vulkan, "data/shaders/command_line_background.frag.spv", &command_line_background_fragment_shader, "Command Line Background Fragment Shader");
    VkPipelineShaderStageCreateInfo command_line_background_shader_infos[2];
    command_line_background_shader_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    command_line_background_shader_infos[0].pNext = NULL;
    command_line_background_shader_infos[0].flags = 0;
    command_line_background_shader_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    command_line_background_shader_infos[0].module = command_line_background_vertex_shader;
    command_line_background_shader_infos[0].pName = "main";
    command_line_background_shader_infos[0].pSpecializationInfo = NULL;
    command_line_background_shader_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    command_line_background_shader_infos[1].pNext = NULL;
    command_line_background_shader_infos[1].flags = 0;
    command_line_background_shader_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    command_line_background_shader_infos[1].module = command_line_background_fragment_shader;
    command_line_background_shader_infos[1].pName = "main";
    command_line_background_shader_infos[1].pSpecializationInfo = NULL;
    VkVertexInputBindingDescription command_line_background_vertex_binding_0;
    command_line_background_vertex_binding_0.binding = 0;
    command_line_background_vertex_binding_0.stride = 2 * sizeof(float);
    command_line_background_vertex_binding_0.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription command_line_background_vertex_attribute_0;
    command_line_background_vertex_attribute_0.location = 0;
    command_line_background_vertex_attribute_0.binding = 0;
    command_line_background_vertex_attribute_0.format = VK_FORMAT_R32G32_SFLOAT;
    command_line_background_vertex_attribute_0.offset = 0;
    VkPipelineVertexInputStateCreateInfo command_line_background_graphics_pipeline_vertex_input_info;
    command_line_background_graphics_pipeline_vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    command_line_background_graphics_pipeline_vertex_input_info.pNext = NULL;
    command_line_background_graphics_pipeline_vertex_input_info.flags = 0;
    command_line_background_graphics_pipeline_vertex_input_info.vertexBindingDescriptionCount = 1;
    command_line_background_graphics_pipeline_vertex_input_info.pVertexBindingDescriptions = &command_line_background_vertex_binding_0;
    command_line_background_graphics_pipeline_vertex_input_info.vertexAttributeDescriptionCount = 1;
    command_line_background_graphics_pipeline_vertex_input_info.pVertexAttributeDescriptions = &command_line_background_vertex_attribute_0;
    VkPipelineInputAssemblyStateCreateInfo command_line_background_graphics_pipeline_input_assembly_info;
    command_line_background_graphics_pipeline_input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    command_line_background_graphics_pipeline_input_assembly_info.pNext = NULL;
    command_line_background_graphics_pipeline_input_assembly_info.flags = 0;
    command_line_background_graphics_pipeline_input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    command_line_background_graphics_pipeline_input_assembly_info.primitiveRestartEnable = VK_FALSE;
    VkViewport command_line_background_viewport;
    command_line_background_viewport.x = 0.0f;
    command_line_background_viewport.y = 0.0f;
    command_line_background_viewport.width = (float)vulkan->surface_caps.currentExtent.width;
    command_line_background_viewport.height = (float)vulkan->surface_caps.currentExtent.height;
    command_line_background_viewport.minDepth = 0.0f;
    command_line_background_viewport.maxDepth = 1.0f;
    VkRect2D command_line_background_scissor;
    command_line_background_scissor.offset.x = (uint32_t)command_line_background_viewport.x;
    command_line_background_scissor.offset.y = (uint32_t)command_line_background_viewport.y;
    command_line_background_scissor.extent.width = (uint32_t)command_line_background_viewport.width;
    command_line_background_scissor.extent.height = (uint32_t)command_line_background_viewport.height;
    VkPipelineViewportStateCreateInfo command_line_background_graphics_pipeline_viewport_info;
    command_line_background_graphics_pipeline_viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    command_line_background_graphics_pipeline_viewport_info.pNext = NULL;
    command_line_background_graphics_pipeline_viewport_info.flags = 0;
    command_line_background_graphics_pipeline_viewport_info.viewportCount = 1;
    command_line_background_graphics_pipeline_viewport_info.pViewports = &command_line_background_viewport;
    command_line_background_graphics_pipeline_viewport_info.scissorCount = 1;
    command_line_background_graphics_pipeline_viewport_info.pScissors = &command_line_background_scissor;
    VkPipelineRasterizationStateCreateInfo command_line_background_graphics_pipeline_rasterization_info;
    command_line_background_graphics_pipeline_rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    command_line_background_graphics_pipeline_rasterization_info.pNext = NULL;
    command_line_background_graphics_pipeline_rasterization_info.flags = 0;
    command_line_background_graphics_pipeline_rasterization_info.depthClampEnable = VK_FALSE;
    command_line_background_graphics_pipeline_rasterization_info.rasterizerDiscardEnable = VK_FALSE;
    command_line_background_graphics_pipeline_rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
    command_line_background_graphics_pipeline_rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
    command_line_background_graphics_pipeline_rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    command_line_background_graphics_pipeline_rasterization_info.depthBiasEnable = VK_FALSE;
    command_line_background_graphics_pipeline_rasterization_info.depthBiasConstantFactor = 0.0f;
    command_line_background_graphics_pipeline_rasterization_info.depthBiasClamp = 0.0f;
    command_line_background_graphics_pipeline_rasterization_info.depthBiasSlopeFactor = 0.0f;
    command_line_background_graphics_pipeline_rasterization_info.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo command_line_background_graphics_pipeline_multisample_info;
    command_line_background_graphics_pipeline_multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    command_line_background_graphics_pipeline_multisample_info.pNext = NULL;
    command_line_background_graphics_pipeline_multisample_info.flags = 0;
    command_line_background_graphics_pipeline_multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    command_line_background_graphics_pipeline_multisample_info.sampleShadingEnable = VK_FALSE;
    command_line_background_graphics_pipeline_multisample_info.pSampleMask = NULL;
    command_line_background_graphics_pipeline_multisample_info.alphaToCoverageEnable = VK_FALSE;
    command_line_background_graphics_pipeline_multisample_info.alphaToOneEnable = VK_FALSE;
    VkPipelineDepthStencilStateCreateInfo command_line_background_graphics_pipeline_depth_info;
    command_line_background_graphics_pipeline_depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    command_line_background_graphics_pipeline_depth_info.pNext = NULL;
    command_line_background_graphics_pipeline_depth_info.flags = 0;
    command_line_background_graphics_pipeline_depth_info.depthTestEnable = VK_FALSE;
    command_line_background_graphics_pipeline_depth_info.depthWriteEnable = VK_FALSE;
    command_line_background_graphics_pipeline_depth_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    command_line_background_graphics_pipeline_depth_info.depthBoundsTestEnable = VK_FALSE;
    command_line_background_graphics_pipeline_depth_info.stencilTestEnable = VK_FALSE;
    command_line_background_graphics_pipeline_depth_info.front.failOp = VK_STENCIL_OP_KEEP;
    command_line_background_graphics_pipeline_depth_info.front.passOp = VK_STENCIL_OP_KEEP;
    command_line_background_graphics_pipeline_depth_info.front.depthFailOp = VK_STENCIL_OP_KEEP;
    command_line_background_graphics_pipeline_depth_info.front.compareOp = VK_COMPARE_OP_NEVER;
    command_line_background_graphics_pipeline_depth_info.front.compareMask = 0x0;
    command_line_background_graphics_pipeline_depth_info.front.writeMask = 0x0;
    command_line_background_graphics_pipeline_depth_info.front.reference = 0x0;
    command_line_background_graphics_pipeline_depth_info.back.failOp = VK_STENCIL_OP_KEEP;
    command_line_background_graphics_pipeline_depth_info.back.passOp = VK_STENCIL_OP_KEEP;
    command_line_background_graphics_pipeline_depth_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
    command_line_background_graphics_pipeline_depth_info.back.compareOp = VK_COMPARE_OP_NEVER;
    command_line_background_graphics_pipeline_depth_info.back.compareMask = 0x0;
    command_line_background_graphics_pipeline_depth_info.back.writeMask = 0x0;
    command_line_background_graphics_pipeline_depth_info.back.reference = 0x0;
    command_line_background_graphics_pipeline_depth_info.minDepthBounds = 0.0f;
    command_line_background_graphics_pipeline_depth_info.maxDepthBounds = 1.0f;
    VkPipelineColorBlendAttachmentState command_line_background_color_blend_attachment_0;
    command_line_background_color_blend_attachment_0.blendEnable = VK_TRUE;
    command_line_background_color_blend_attachment_0.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    command_line_background_color_blend_attachment_0.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    command_line_background_color_blend_attachment_0.colorBlendOp = VK_BLEND_OP_ADD;
    command_line_background_color_blend_attachment_0.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    command_line_background_color_blend_attachment_0.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    command_line_background_color_blend_attachment_0.alphaBlendOp = VK_BLEND_OP_ADD;
    command_line_background_color_blend_attachment_0.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo command_line_background_graphics_pipeline_color_blend_info;
    command_line_background_graphics_pipeline_color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    command_line_background_graphics_pipeline_color_blend_info.pNext = NULL;
    command_line_background_graphics_pipeline_color_blend_info.flags = 0;
    command_line_background_graphics_pipeline_color_blend_info.logicOpEnable = VK_FALSE;
    command_line_background_graphics_pipeline_color_blend_info.attachmentCount = 1;
    command_line_background_graphics_pipeline_color_blend_info.pAttachments = &command_line_background_color_blend_attachment_0;
    command_line_background_graphics_pipeline_color_blend_info.blendConstants[0] = 1.0f;
    command_line_background_graphics_pipeline_color_blend_info.blendConstants[1] = 1.0f;
    command_line_background_graphics_pipeline_color_blend_info.blendConstants[2] = 1.0f;
    command_line_background_graphics_pipeline_color_blend_info.blendConstants[3] = 1.0f;
    VkPipelineDynamicStateCreateInfo command_line_background_graphics_pipeline_dynamic_info;
    command_line_background_graphics_pipeline_dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    command_line_background_graphics_pipeline_dynamic_info.pNext = NULL;
    command_line_background_graphics_pipeline_dynamic_info.flags = 0;
    command_line_background_graphics_pipeline_dynamic_info.dynamicStateCount = 0;
    command_line_background_graphics_pipeline_dynamic_info.pDynamicStates = NULL;
    VkPushConstantRange command_line_background_push_constant;
    command_line_background_push_constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    command_line_background_push_constant.offset = 0;
    command_line_background_push_constant.size = 4 * sizeof(float);
    VkPipelineLayoutCreateInfo command_line_background_graphics_pipeline_layout_info;
    command_line_background_graphics_pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    command_line_background_graphics_pipeline_layout_info.pNext = NULL;
    command_line_background_graphics_pipeline_layout_info.flags = 0;
    command_line_background_graphics_pipeline_layout_info.setLayoutCount = 0;
    command_line_background_graphics_pipeline_layout_info.pSetLayouts = NULL;
    command_line_background_graphics_pipeline_layout_info.pushConstantRangeCount = 1;
    command_line_background_graphics_pipeline_layout_info.pPushConstantRanges = &command_line_background_push_constant;
    VK_CHECK_RES(vkCreatePipelineLayout(vulkan->device, &command_line_background_graphics_pipeline_layout_info, NULL, &command_line_background_graphics_pipeline_layout));
    VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)command_line_background_graphics_pipeline_layout, "Command Line Background Graphics Pipeline Layout (Main Render Pass)");
    VkGraphicsPipelineCreateInfo command_line_background_graphics_pipeline_info;
    command_line_background_graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    command_line_background_graphics_pipeline_info.pNext = &pipeline_rendering_info;
    command_line_background_graphics_pipeline_info.flags = 0;
    command_line_background_graphics_pipeline_info.stageCount = 2;
    command_line_background_graphics_pipeline_info.pStages = command_line_background_shader_infos;
    command_line_background_graphics_pipeline_info.pVertexInputState = &command_line_background_graphics_pipeline_vertex_input_info;
    command_line_background_graphics_pipeline_info.pInputAssemblyState = &command_line_background_graphics_pipeline_input_assembly_info;
    command_line_background_graphics_pipeline_info.pTessellationState = NULL;
    command_line_background_graphics_pipeline_info.pViewportState = &command_line_background_graphics_pipeline_viewport_info;
    command_line_background_graphics_pipeline_info.pRasterizationState = &command_line_background_graphics_pipeline_rasterization_info;
    command_line_background_graphics_pipeline_info.pMultisampleState = &command_line_background_graphics_pipeline_multisample_info;
    command_line_background_graphics_pipeline_info.pDepthStencilState = &command_line_background_graphics_pipeline_depth_info;
    command_line_background_graphics_pipeline_info.pColorBlendState = &command_line_background_graphics_pipeline_color_blend_info;
    command_line_background_graphics_pipeline_info.pDynamicState = &command_line_background_graphics_pipeline_dynamic_info;
    command_line_background_graphics_pipeline_info.layout = command_line_background_graphics_pipeline_layout;
    //command_line_background_graphics_pipeline_info.renderPass = render_pass;
    command_line_background_graphics_pipeline_info.renderPass = VK_NULL_HANDLE;
    command_line_background_graphics_pipeline_info.subpass = 0;
    command_line_background_graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    command_line_background_graphics_pipeline_info.basePipelineIndex = -1;
    VK_CHECK_RES(vkCreateGraphicsPipelines(vulkan->device, vulkan->pipeline_cache, 1, &command_line_background_graphics_pipeline_info, NULL, &command_line_background_graphics_pipeline));
    VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_PIPELINE, (uint64_t)command_line_background_graphics_pipeline, "Command Line Background Graphics Pipeline (Main Render Pass)");

    // Command line text graphics pipeline
    VulkanCreateShader(vulkan, "data/shaders/command_line_text.vert.spv", &command_line_text_vertex_shader, "Command Line Text Vertex Shader");
    VulkanCreateShader(vulkan, "data/shaders/command_line_text.frag.spv", &command_line_text_fragment_shader, "Command Line Text Fragment Shader");
    VkPipelineShaderStageCreateInfo command_line_text_shader_infos[2];
    command_line_text_shader_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    command_line_text_shader_infos[0].pNext = NULL;
    command_line_text_shader_infos[0].flags = 0;
    command_line_text_shader_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    command_line_text_shader_infos[0].module = command_line_text_vertex_shader;
    command_line_text_shader_infos[0].pName = "main";
    command_line_text_shader_infos[0].pSpecializationInfo = NULL;
    command_line_text_shader_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    command_line_text_shader_infos[1].pNext = NULL;
    command_line_text_shader_infos[1].flags = 0;
    command_line_text_shader_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    command_line_text_shader_infos[1].module = command_line_text_fragment_shader;
    command_line_text_shader_infos[1].pName = "main";
    command_line_text_shader_infos[1].pSpecializationInfo = NULL;
    VkVertexInputBindingDescription command_line_text_vertex_binding_0;
    command_line_text_vertex_binding_0.binding = 0;
    command_line_text_vertex_binding_0.stride = 4 * sizeof(float);
    command_line_text_vertex_binding_0.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription command_line_text_vertex_attributes[2];
    // Position
    command_line_text_vertex_attributes[0].location = 0;
    command_line_text_vertex_attributes[0].binding = 0;
    command_line_text_vertex_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
    command_line_text_vertex_attributes[0].offset = 0;
    // UV
    command_line_text_vertex_attributes[1].location = 1;
    command_line_text_vertex_attributes[1].binding = 0;
    command_line_text_vertex_attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    command_line_text_vertex_attributes[1].offset = 2 * sizeof(float);
    VkPipelineVertexInputStateCreateInfo command_line_text_graphics_pipeline_vertex_input_info;
    command_line_text_graphics_pipeline_vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    command_line_text_graphics_pipeline_vertex_input_info.pNext = NULL;
    command_line_text_graphics_pipeline_vertex_input_info.flags = 0;
    command_line_text_graphics_pipeline_vertex_input_info.vertexBindingDescriptionCount = 1;
    command_line_text_graphics_pipeline_vertex_input_info.pVertexBindingDescriptions = &command_line_text_vertex_binding_0;
    command_line_text_graphics_pipeline_vertex_input_info.vertexAttributeDescriptionCount = 2;
    command_line_text_graphics_pipeline_vertex_input_info.pVertexAttributeDescriptions = command_line_text_vertex_attributes;
    VkPipelineInputAssemblyStateCreateInfo command_line_text_graphics_pipeline_input_assembly_info;
    command_line_text_graphics_pipeline_input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    command_line_text_graphics_pipeline_input_assembly_info.pNext = NULL;
    command_line_text_graphics_pipeline_input_assembly_info.flags = 0;
    command_line_text_graphics_pipeline_input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    command_line_text_graphics_pipeline_input_assembly_info.primitiveRestartEnable = VK_FALSE;
    VkViewport command_line_text_viewport;
    command_line_text_viewport.x = 0.0f;
    command_line_text_viewport.y = 0.0f;
    command_line_text_viewport.width = (float)vulkan->surface_caps.currentExtent.width;
    command_line_text_viewport.height = (float)vulkan->surface_caps.currentExtent.height;
    command_line_text_viewport.minDepth = 0.0f;
    command_line_text_viewport.maxDepth = 1.0f;
    VkRect2D command_line_text_scissor;
    command_line_text_scissor.offset.x = (uint32_t)command_line_text_viewport.x;
    command_line_text_scissor.offset.y = (uint32_t)command_line_text_viewport.y;
    command_line_text_scissor.extent.width = (uint32_t)command_line_text_viewport.width;
    command_line_text_scissor.extent.height = (uint32_t)command_line_text_viewport.height;
    VkPipelineViewportStateCreateInfo command_line_text_graphics_pipeline_viewport_info;
    command_line_text_graphics_pipeline_viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    command_line_text_graphics_pipeline_viewport_info.pNext = NULL;
    command_line_text_graphics_pipeline_viewport_info.flags = 0;
    command_line_text_graphics_pipeline_viewport_info.viewportCount = 1;
    command_line_text_graphics_pipeline_viewport_info.pViewports = &command_line_text_viewport;
    command_line_text_graphics_pipeline_viewport_info.scissorCount = 1;
    command_line_text_graphics_pipeline_viewport_info.pScissors = &command_line_text_scissor;
    VkPipelineRasterizationStateCreateInfo command_line_text_graphics_pipeline_rasterization_info;
    command_line_text_graphics_pipeline_rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    command_line_text_graphics_pipeline_rasterization_info.pNext = NULL;
    command_line_text_graphics_pipeline_rasterization_info.flags = 0;
    command_line_text_graphics_pipeline_rasterization_info.depthClampEnable = VK_FALSE;
    command_line_text_graphics_pipeline_rasterization_info.rasterizerDiscardEnable = VK_FALSE;
    command_line_text_graphics_pipeline_rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
    command_line_text_graphics_pipeline_rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
    command_line_text_graphics_pipeline_rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    command_line_text_graphics_pipeline_rasterization_info.depthBiasEnable = VK_FALSE;
    command_line_text_graphics_pipeline_rasterization_info.depthBiasConstantFactor = 0.0f;
    command_line_text_graphics_pipeline_rasterization_info.depthBiasClamp = 0.0f;
    command_line_text_graphics_pipeline_rasterization_info.depthBiasSlopeFactor = 0.0f;
    command_line_text_graphics_pipeline_rasterization_info.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo command_line_text_graphics_pipeline_multisample_info;
    command_line_text_graphics_pipeline_multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    command_line_text_graphics_pipeline_multisample_info.pNext = NULL;
    command_line_text_graphics_pipeline_multisample_info.flags = 0;
    command_line_text_graphics_pipeline_multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    command_line_text_graphics_pipeline_multisample_info.sampleShadingEnable = VK_FALSE;
    command_line_text_graphics_pipeline_multisample_info.pSampleMask = NULL;
    command_line_text_graphics_pipeline_multisample_info.alphaToCoverageEnable = VK_FALSE;
    command_line_text_graphics_pipeline_multisample_info.alphaToOneEnable = VK_FALSE;
    VkPipelineDepthStencilStateCreateInfo command_line_text_graphics_pipeline_depth_info;
    command_line_text_graphics_pipeline_depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    command_line_text_graphics_pipeline_depth_info.pNext = NULL;
    command_line_text_graphics_pipeline_depth_info.flags = 0;
    command_line_text_graphics_pipeline_depth_info.depthTestEnable = VK_FALSE;
    command_line_text_graphics_pipeline_depth_info.depthWriteEnable = VK_FALSE;
    command_line_text_graphics_pipeline_depth_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    command_line_text_graphics_pipeline_depth_info.depthBoundsTestEnable = VK_FALSE;
    command_line_text_graphics_pipeline_depth_info.stencilTestEnable = VK_FALSE;
    command_line_text_graphics_pipeline_depth_info.front.failOp = VK_STENCIL_OP_KEEP;
    command_line_text_graphics_pipeline_depth_info.front.passOp = VK_STENCIL_OP_KEEP;
    command_line_text_graphics_pipeline_depth_info.front.depthFailOp = VK_STENCIL_OP_KEEP;
    command_line_text_graphics_pipeline_depth_info.front.compareOp = VK_COMPARE_OP_NEVER;
    command_line_text_graphics_pipeline_depth_info.front.compareMask = 0x0;
    command_line_text_graphics_pipeline_depth_info.front.writeMask = 0x0;
    command_line_text_graphics_pipeline_depth_info.front.reference = 0x0;
    command_line_text_graphics_pipeline_depth_info.back.failOp = VK_STENCIL_OP_KEEP;
    command_line_text_graphics_pipeline_depth_info.back.passOp = VK_STENCIL_OP_KEEP;
    command_line_text_graphics_pipeline_depth_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
    command_line_text_graphics_pipeline_depth_info.back.compareOp = VK_COMPARE_OP_NEVER;
    command_line_text_graphics_pipeline_depth_info.back.compareMask = 0x0;
    command_line_text_graphics_pipeline_depth_info.back.writeMask = 0x0;
    command_line_text_graphics_pipeline_depth_info.back.reference = 0x0;
    command_line_text_graphics_pipeline_depth_info.minDepthBounds = 0.0f;
    command_line_text_graphics_pipeline_depth_info.maxDepthBounds = 1.0f;
    VkPipelineColorBlendAttachmentState command_line_text_color_blend_attachment_0;
    command_line_text_color_blend_attachment_0.blendEnable = VK_TRUE;
    command_line_text_color_blend_attachment_0.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    command_line_text_color_blend_attachment_0.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    command_line_text_color_blend_attachment_0.colorBlendOp = VK_BLEND_OP_ADD;
    command_line_text_color_blend_attachment_0.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    command_line_text_color_blend_attachment_0.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    command_line_text_color_blend_attachment_0.alphaBlendOp = VK_BLEND_OP_ADD;
    command_line_text_color_blend_attachment_0.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo command_line_text_graphics_pipeline_color_blend_info;
    command_line_text_graphics_pipeline_color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    command_line_text_graphics_pipeline_color_blend_info.pNext = NULL;
    command_line_text_graphics_pipeline_color_blend_info.flags = 0;
    command_line_text_graphics_pipeline_color_blend_info.logicOpEnable = VK_FALSE;
    command_line_text_graphics_pipeline_color_blend_info.attachmentCount = 1;
    command_line_text_graphics_pipeline_color_blend_info.pAttachments = &command_line_text_color_blend_attachment_0;
    command_line_text_graphics_pipeline_color_blend_info.blendConstants[0] = 1.0f;
    command_line_text_graphics_pipeline_color_blend_info.blendConstants[1] = 1.0f;
    command_line_text_graphics_pipeline_color_blend_info.blendConstants[2] = 1.0f;
    command_line_text_graphics_pipeline_color_blend_info.blendConstants[3] = 1.0f;
    VkPipelineDynamicStateCreateInfo command_line_text_graphics_pipeline_dynamic_info;
    command_line_text_graphics_pipeline_dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    command_line_text_graphics_pipeline_dynamic_info.pNext = NULL;
    command_line_text_graphics_pipeline_dynamic_info.flags = 0;
    command_line_text_graphics_pipeline_dynamic_info.dynamicStateCount = 0;
    command_line_text_graphics_pipeline_dynamic_info.pDynamicStates = NULL;
    VkPushConstantRange command_line_text_push_constant;
    command_line_text_push_constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    command_line_text_push_constant.offset = 0;
    command_line_text_push_constant.size = 4 * sizeof(float);
    VkPipelineLayoutCreateInfo command_line_text_graphics_pipeline_layout_info;
    command_line_text_graphics_pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    command_line_text_graphics_pipeline_layout_info.pNext = NULL;
    command_line_text_graphics_pipeline_layout_info.flags = 0;
    command_line_text_graphics_pipeline_layout_info.setLayoutCount = 1;
    command_line_text_graphics_pipeline_layout_info.pSetLayouts = &font_image_sampler_descriptor_set_layout;
    command_line_text_graphics_pipeline_layout_info.pushConstantRangeCount = 1;
    command_line_text_graphics_pipeline_layout_info.pPushConstantRanges = &command_line_text_push_constant;
    VK_CHECK_RES(vkCreatePipelineLayout(vulkan->device, &command_line_text_graphics_pipeline_layout_info, NULL, &command_line_text_graphics_pipeline_layout));
    VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)command_line_text_graphics_pipeline_layout, "Command Line Text Graphics Pipeline Layout (Main Render Pass)");
    VkGraphicsPipelineCreateInfo command_line_text_graphics_pipeline_info;
    command_line_text_graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    command_line_text_graphics_pipeline_info.pNext = &pipeline_rendering_info;
    command_line_text_graphics_pipeline_info.flags = 0;
    command_line_text_graphics_pipeline_info.stageCount = 2;
    command_line_text_graphics_pipeline_info.pStages = command_line_text_shader_infos;
    command_line_text_graphics_pipeline_info.pVertexInputState = &command_line_text_graphics_pipeline_vertex_input_info;
    command_line_text_graphics_pipeline_info.pInputAssemblyState = &command_line_text_graphics_pipeline_input_assembly_info;
    command_line_text_graphics_pipeline_info.pTessellationState = NULL;
    command_line_text_graphics_pipeline_info.pViewportState = &command_line_text_graphics_pipeline_viewport_info;
    command_line_text_graphics_pipeline_info.pRasterizationState = &command_line_text_graphics_pipeline_rasterization_info;
    command_line_text_graphics_pipeline_info.pMultisampleState = &command_line_text_graphics_pipeline_multisample_info;
    command_line_text_graphics_pipeline_info.pDepthStencilState = &command_line_text_graphics_pipeline_depth_info;
    command_line_text_graphics_pipeline_info.pColorBlendState = &command_line_text_graphics_pipeline_color_blend_info;
    command_line_text_graphics_pipeline_info.pDynamicState = &command_line_text_graphics_pipeline_dynamic_info;
    command_line_text_graphics_pipeline_info.layout = command_line_text_graphics_pipeline_layout;
    //command_line_text_graphics_pipeline_info.renderPass = render_pass;
    command_line_text_graphics_pipeline_info.renderPass = VK_NULL_HANDLE;
    command_line_text_graphics_pipeline_info.subpass = 0;
    command_line_text_graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    command_line_text_graphics_pipeline_info.basePipelineIndex = -1;
    VK_CHECK_RES(vkCreateGraphicsPipelines(vulkan->device, vulkan->pipeline_cache, 1, &command_line_text_graphics_pipeline_info, NULL, &command_line_text_graphics_pipeline));
    VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_PIPELINE, (uint64_t)command_line_text_graphics_pipeline, "Command Line Text Graphics Pipeline (Main Render Pass)");
    
    resolution[0] = (float)vulkan->surface_caps.currentExtent.width;
    resolution[1] = (float)vulkan->surface_caps.currentExtent.height;
}

void SceneUIRecreateFramebuffers(vulkan_context_t* vulkan)
{
    resolution[0] = (float)vulkan->surface_caps.currentExtent.width;
    resolution[1] = (float)vulkan->surface_caps.currentExtent.height;
}

static void InfoSectionSetErrorMessage(const char* error_message, char* info_section_text, size_t* info_section_text_string_length)
{
    // +8 to skip " Error: "
    strcpy(info_section_text + 8, error_message);
    *info_section_text_string_length = strlen(info_section_text);
}

void SceneUIUpdateInfoMessage(const char* message, uint32_t row)
{
    uint32_t offset = 0;
    switch (row)
    {
        case INFO_SECTION_ROW_TITLE:
        {
            offset = 8; // Skip " Error: "
        } break;
        
        case INFO_SECTION_ROW_LOOP:
        {
            offset = 12; // Skip " Loop mode: "
        } break;
        
        case INFO_SECTION_ROW_SHUFFLE:
        {
            offset = 15; // Skip " Shuffle mode: "
        } break;
        
        case INFO_SECTION_ROW_PLAYLIST:
        {
            offset = 11; // Skip " Playlist: "
        } break;
        
        case INFO_SECTION_ROW_SONG:
        {
            offset = 7; // Skip " Song: "
        } break;
        
        case INFO_SECTION_ROW_ARTIST:
        {
            offset = 9; // Skip " Artist: "
        } break;
        
        case INFO_SECTION_ROW_ALBUM:
        {
            offset = 8; // Skip " Album: "
        } break;
        
        case INFO_SECTION_ROW_CHANNEL_COUNT:
        {
            offset = 16; // Skip " Channel count: "
        } break;
        
        case INFO_SECTION_ROW_SAMPLE_RATE:
        {
            offset = 14; // Skip " Sample rate: "
        } break;

        case INFO_SECTION_ROW_BITS_PER_SAMPLE:
        {
            offset = 18; // Skip " Bits per sample: "
        } break;
        
        case INFO_SECTION_ROW_ERROR:
        {
            offset = 8; // Skip " Error: "
        } break;

        default:
        {
            assert(0);
        }
    }
    strcpy(info_section_texts_rows[row] + offset, message);
    info_section_texts_row_string_lengths[row] = strlen(info_section_texts_rows[row]);
}

void SceneUIRender(vulkan_context_t* vulkan, VkCommandBuffer frame_command_buffer, uint32_t frame_image_index, uint32_t frame_resource_index, bool ui_showing, char* sound_player_command_string, uint16_t sound_player_command_string_length)
{
    // UI render pass
    // No synchronization is neccessary between the scene's render pass(es) and the UI render pass, as the UI render pass
    // only writes to the intermediate swapchain image, which the scene's render pass(es) also do
    VulkanCmdBeginDebugUtilsLabel(vulkan, frame_command_buffer, "UI Render Pass");
    
    if (ui_showing)
    {
        VkRenderingAttachmentInfo color_attachment;
        color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment.pNext = NULL;
        color_attachment.imageView = vulkan->intermediate_swapchain_image_view;
        color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment.resolveMode = VK_RESOLVE_MODE_NONE;
        color_attachment.resolveImageView = VK_NULL_HANDLE;
        color_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.clearValue.color.float32[0] = 0.0f;
        color_attachment.clearValue.color.float32[1] = 0.0f;
        color_attachment.clearValue.color.float32[2] = 0.0f;
        color_attachment.clearValue.color.float32[3] = 1.0f;
        VkRenderingInfo rendering_info;
        rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rendering_info.pNext = NULL;
        rendering_info.flags = 0;
        rendering_info.renderArea.extent.width = resolution[0];
        rendering_info.renderArea.extent.height = resolution[1];
        rendering_info.renderArea.offset.x = 0;
        rendering_info.renderArea.offset.y = 0;
        rendering_info.layerCount = 1;
        rendering_info.viewMask = 0;
        rendering_info.colorAttachmentCount = 1;
        rendering_info.pColorAttachments = &color_attachment;
        rendering_info.pDepthAttachment = NULL;
        rendering_info.pStencilAttachment = NULL;
        vkCmdBeginRendering(frame_command_buffer, &rendering_info);

        // Info section background
        vkCmdBindPipeline(frame_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, command_line_background_graphics_pipeline);
        vkCmdPushConstants(frame_command_buffer, command_line_background_graphics_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), &info_section_background_color);
        VkDeviceSize info_section_background_vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(frame_command_buffer, 0, 1, &info_section_background_vertex_buffer, &info_section_background_vertex_buffer_offset);
        vkCmdDraw(frame_command_buffer, 6, 1, 0, 0);

        // Command line background
        vkCmdBindPipeline(frame_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, command_line_background_graphics_pipeline);
        vkCmdPushConstants(frame_command_buffer, command_line_background_graphics_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), &command_line_background_color);
        VkDeviceSize command_line_background_vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(frame_command_buffer, 0, 1, &command_line_background_vertex_buffer, &command_line_background_vertex_buffer_offset);
        vkCmdDraw(frame_command_buffer, 6, 1, 0, 0);

        // Info section text
        float info_section_row_top_left_y = info_section_top_left_y;
        float* info_section_text_buffer_ptr = NULL;
        vkMapMemory(vulkan->device, info_section_text_vertex_buffer_memories[frame_resource_index], 0, VK_WHOLE_SIZE, 0, (void**)(&info_section_text_buffer_ptr));
        for (uint8_t i = 0; i < INFO_SECTION_ROW_COUNT; i++)
        {
            info_section_texts_row_string_length_to_render[i] = 0;
            uint64_t info_section_text_buffer_offset = i * MAX_PATH * 6 * 4;
            float info_section_text_left_x = info_section_top_left_x;
            for (uint32_t j = 0; j < info_section_texts_row_string_lengths[i]; j++)
            {
                stb_fontchar* char_data = &font_character_data[(uint32_t)info_section_texts_rows[i][j] - STB_SOMEFONT_FIRST_CHAR];

                // (x0,y0) and (s0,t0) is top-left
                // (x1,y1) and (s1,t1) is bottom-right
                float char_x_left   = info_section_text_left_x + (((char_data->x0f * info_section_font_scale) / (float)vulkan->surface_caps.currentExtent.width)  * 2.0f);
                float char_x_right  = info_section_text_left_x + (((char_data->x1f * info_section_font_scale) / (float)vulkan->surface_caps.currentExtent.width)  * 2.0f);
                float char_y_top    = info_section_row_top_left_y  + (((char_data->y0f * info_section_font_scale) / (float)vulkan->surface_caps.currentExtent.height) * 2.0f);
                float char_y_bottom = info_section_row_top_left_y  + (((char_data->y1f * info_section_font_scale) / (float)vulkan->surface_caps.currentExtent.height) * 2.0f);
                float char_advance  = ((char_data->advance * info_section_font_scale) / (float)vulkan->surface_caps.currentExtent.width) * 2.0f;

                // Check that string length will still within the info sections width
                if (((info_section_text_left_x - info_section_top_left_x) + char_advance) > info_section_width_ndc)
                {
                    info_section_texts_row_string_length_to_render[i] = j;
                    break;
                }
                info_section_texts_row_string_length_to_render[i] = j + 1;

                // Top left
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_x_left;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_y_top;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_data->s0f;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_data->t0f;

                // Bottom left
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_x_left;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_y_bottom;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_data->s0f;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_data->t1f;

                // Bottom right
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_x_right;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_y_bottom;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_data->s1f;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_data->t1f;

                // Bottom right
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_x_right;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_y_bottom;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_data->s1f;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_data->t1f;

                // Top right
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_x_right;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_y_top;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_data->s1f;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_data->t0f;

                // Top left
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_x_left;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_y_top;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_data->s0f;
                info_section_text_buffer_ptr[info_section_text_buffer_offset++] = char_data->t0f;

                info_section_text_left_x += char_advance;
            }

            info_section_row_top_left_y += info_section_row_height_ndc;
        }
        vkUnmapMemory(vulkan->device, info_section_text_vertex_buffer_memories[frame_resource_index]);

        // Draw text
        vkCmdBindPipeline(frame_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, command_line_text_graphics_pipeline);
        vkCmdBindDescriptorSets(frame_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, command_line_text_graphics_pipeline_layout, 0, 1, &font_image_sampler_descriptor_set, 0, NULL);
        vkCmdPushConstants(frame_command_buffer, command_line_text_graphics_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), &info_section_text_color);
        for (uint8_t i = 0; i < INFO_SECTION_ROW_COUNT; i++)
        {
            VkDeviceSize info_section_text_vertex_buffer_offset = i * MAX_PATH * 6 * 4 * sizeof(float);
            vkCmdBindVertexBuffers(frame_command_buffer, 0, 1, &info_section_text_vertex_buffers[frame_resource_index], &info_section_text_vertex_buffer_offset);
            vkCmdDraw(frame_command_buffer, info_section_texts_row_string_length_to_render[i] * 6, 1, 0, 0);
        }

        // Command line text
        // Compute how many character will be part of text
        uint32_t command_line_text_start_index = 0;
        float command_line_text_width = 0.0f;
        for (int32_t i = sound_player_command_string_length - 1; i >= 0; i--)
        {
            stb_fontchar* char_data = &font_character_data[(uint32_t)sound_player_command_string[i] - STB_SOMEFONT_FIRST_CHAR];
            float char_advance  = ((char_data->advance * command_line_font_scale) / (float)vulkan->surface_caps.currentExtent.width) * 2.0f;
            command_line_text_width += char_advance;
            if (command_line_text_width > command_line_width_ndc)
            {
                assert(i < (sound_player_command_string_length - 1)); // Single character doesn't fit in command line's width
                command_line_text_start_index = (uint32_t)(i + 1);
                break;
            }
        }

        // Build text
        float command_line_text_left_x = command_line_top_left_x;
        uint32_t command_line_text_index = 0;
        float* command_line_text_buffer_ptr = NULL;
        vkMapMemory(vulkan->device, command_line_text_vertex_buffer_memories[frame_resource_index], 0, VK_WHOLE_SIZE, 0, (void**)(&command_line_text_buffer_ptr));
        for (uint32_t i = command_line_text_start_index; i < sound_player_command_string_length; i++)
        {
            stb_fontchar* char_data = &font_character_data[(uint32_t)sound_player_command_string[i] - STB_SOMEFONT_FIRST_CHAR];

            // (x0,y0) and (s0,t0) is top-left
            // (x1,y1) and (s1,t1) is bottom-right
            float char_x_left   = command_line_text_left_x + (((char_data->x0f * command_line_font_scale) / (float)vulkan->surface_caps.currentExtent.width)  * 2.0f);
            float char_x_right  = command_line_text_left_x + (((char_data->x1f * command_line_font_scale) / (float)vulkan->surface_caps.currentExtent.width)  * 2.0f);
            float char_y_top    = command_line_top_left_y  + (((char_data->y0f * command_line_font_scale) / (float)vulkan->surface_caps.currentExtent.height) * 2.0f);
            float char_y_bottom = command_line_top_left_y  + (((char_data->y1f * command_line_font_scale) / (float)vulkan->surface_caps.currentExtent.height) * 2.0f);
            float char_advance  = ((char_data->advance * command_line_font_scale) / (float)vulkan->surface_caps.currentExtent.width) * 2.0f;

            // Top left
            command_line_text_buffer_ptr[command_line_text_index++] = char_x_left;
            command_line_text_buffer_ptr[command_line_text_index++] = char_y_top;
            command_line_text_buffer_ptr[command_line_text_index++] = char_data->s0f;
            command_line_text_buffer_ptr[command_line_text_index++] = char_data->t0f;

            // Bottom left
            command_line_text_buffer_ptr[command_line_text_index++] = char_x_left;
            command_line_text_buffer_ptr[command_line_text_index++] = char_y_bottom;
            command_line_text_buffer_ptr[command_line_text_index++] = char_data->s0f;
            command_line_text_buffer_ptr[command_line_text_index++] = char_data->t1f;

            // Bottom right
            command_line_text_buffer_ptr[command_line_text_index++] = char_x_right;
            command_line_text_buffer_ptr[command_line_text_index++] = char_y_bottom;
            command_line_text_buffer_ptr[command_line_text_index++] = char_data->s1f;
            command_line_text_buffer_ptr[command_line_text_index++] = char_data->t1f;

            // Bottom right
            command_line_text_buffer_ptr[command_line_text_index++] = char_x_right;
            command_line_text_buffer_ptr[command_line_text_index++] = char_y_bottom;
            command_line_text_buffer_ptr[command_line_text_index++] = char_data->s1f;
            command_line_text_buffer_ptr[command_line_text_index++] = char_data->t1f;

            // Top right
            command_line_text_buffer_ptr[command_line_text_index++] = char_x_right;
            command_line_text_buffer_ptr[command_line_text_index++] = char_y_top;
            command_line_text_buffer_ptr[command_line_text_index++] = char_data->s1f;
            command_line_text_buffer_ptr[command_line_text_index++] = char_data->t0f;

            // Top left
            command_line_text_buffer_ptr[command_line_text_index++] = char_x_left;
            command_line_text_buffer_ptr[command_line_text_index++] = char_y_top;
            command_line_text_buffer_ptr[command_line_text_index++] = char_data->s0f;
            command_line_text_buffer_ptr[command_line_text_index++] = char_data->t0f;

            command_line_text_left_x += char_advance;
        }
        vkUnmapMemory(vulkan->device, command_line_text_vertex_buffer_memories[frame_resource_index]);

        // Draw text
        vkCmdBindPipeline(frame_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, command_line_text_graphics_pipeline);
        vkCmdBindDescriptorSets(frame_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, command_line_text_graphics_pipeline_layout, 0, 1, &font_image_sampler_descriptor_set, 0, NULL);
        vkCmdPushConstants(frame_command_buffer, command_line_text_graphics_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), &command_line_text_color);
        VkDeviceSize command_line_text_vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(frame_command_buffer, 0, 1, &command_line_text_vertex_buffers[frame_resource_index], &command_line_text_vertex_buffer_offset);
        vkCmdDraw(frame_command_buffer, sound_player_command_string_length * 6, 1, 0, 0);
        vkCmdEndRendering(frame_command_buffer);
    }
    
    VulkanCmdEndDebugUtilsLabel(vulkan, frame_command_buffer);
}

void SceneUIDestroy(vulkan_context_t* vulkan)
{
    vkDestroyPipeline(vulkan->device, command_line_text_graphics_pipeline, NULL);
    vkDestroyPipeline(vulkan->device, command_line_background_graphics_pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, command_line_text_graphics_pipeline_layout, NULL);
    vkDestroyPipelineLayout(vulkan->device, command_line_background_graphics_pipeline_layout, NULL);
    vkDestroyShaderModule(vulkan->device, command_line_background_vertex_shader, NULL);
    vkDestroyShaderModule(vulkan->device, command_line_background_fragment_shader, NULL);
    vkDestroyShaderModule(vulkan->device, command_line_text_vertex_shader, NULL);
    vkDestroyShaderModule(vulkan->device, command_line_text_fragment_shader, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, font_image_sampler_descriptor_set_layout, NULL);
    vkDestroyDescriptorPool(vulkan->device, descriptor_pool, NULL);
    vkDestroySampler(vulkan->device, font_image_sampler, NULL);
    vkDestroyImageView(vulkan->device, font_image_view, NULL);
    vkFreeMemory(vulkan->device, font_image_memory, NULL);
    vkDestroyImage(vulkan->device, font_image, NULL);
    for (uint32_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        VulkanDestroyBuffer(vulkan, &command_line_text_vertex_buffers[i], &command_line_text_vertex_buffer_memories[i]);
    }
    VulkanDestroyBuffer(vulkan, &command_line_background_vertex_buffer, &command_line_background_vertex_buffer_memory);
    for (uint32_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        VulkanDestroyBuffer(vulkan, &info_section_text_vertex_buffers[i], &info_section_text_vertex_buffer_memories[i]);
    }
    VulkanDestroyBuffer(vulkan, &info_section_background_vertex_buffer, &info_section_background_vertex_buffer_memory);
}