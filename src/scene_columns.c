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

#include "scene_columns.h"

// Buffers
static VkBuffer fullscreen_vertex_buffer;
static VkDeviceMemory fullscreen_vertex_buffer_memory;

// Render Passes

// Descriptor Pools
static VkDescriptorPool descriptor_pool;

// Descriptor Set Layouts
static VkDescriptorSetLayout dft_storage_buffer_descriptor_set_layout;

// Descriptor Sets
static VkDescriptorSet dft_storage_buffer_descriptor_sets[VULKAN_MAX_FRAMES_IN_FLIGHT];

// Shaders
static VkShaderModule fullscreen_vertex_shader;
static VkShaderModule fullscreen_fragment_shader;

// Graphics Pipeline Layouts
static VkPipelineLayout fullscreen_graphics_pipeline_layout;

// Graphics Pipelines
static VkPipeline fullscreen_graphics_pipeline;

// Viewport resolution
static float resolution[2];

void SceneColumnsInit(vulkan_context_t* vulkan, VkBuffer* dft_storage_buffers)
{
    // Fullscreen quad
    float fullscreen_vertex_buffer_data[24] = {
        // Position        UV
        -1.0f, -1.0f,  0.0f, 1.0f,
        -1.0f, +1.0f,  0.0f, 0.0f,
        +1.0f, +1.0f,  1.0f, 0.0f,

        +1.0f, +1.0f,  1.0f, 0.0f,
        +1.0f, -1.0f,  1.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 1.0f
    };
    VulkanCreateBuffer(vulkan, fullscreen_vertex_buffer_data, 6 * 4 * sizeof(float), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &fullscreen_vertex_buffer, &fullscreen_vertex_buffer_memory, "SceneColumns: Fullscreen Vertex Buffer", "SceneColumns: Fullscreen Vertex Buffer Memory");

    // Render pass
    VkPipelineRenderingCreateInfo pipeline_rendering_info;
    pipeline_rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipeline_rendering_info.pNext = NULL;
    pipeline_rendering_info.viewMask = 0;
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &vulkan->intermediate_swapchain_image_format;
    pipeline_rendering_info.depthAttachmentFormat = vulkan->depth_stencil_format;
    pipeline_rendering_info.stencilAttachmentFormat = vulkan->depth_stencil_format;

    // Descriptor pool
    VkDescriptorPoolSize descriptor_pool_size;
    descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptor_pool_size.descriptorCount = VULKAN_MAX_FRAMES_IN_FLIGHT; // DFT storage buffer
    VkDescriptorPoolCreateInfo descriptor_pool_info;
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = NULL;
    descriptor_pool_info.flags = 0;
    descriptor_pool_info.maxSets = descriptor_pool_size.descriptorCount;
    descriptor_pool_info.poolSizeCount = 1;
    descriptor_pool_info.pPoolSizes = &descriptor_pool_size;
    VK_CHECK_RES(vkCreateDescriptorPool(vulkan->device, &descriptor_pool_info, NULL, &descriptor_pool));
    // DFT storage buffer descriptor set
    VkDescriptorSetLayoutBinding dft_storage_buffer_descriptor_set_layout_binding;
    dft_storage_buffer_descriptor_set_layout_binding.binding = 0;
    dft_storage_buffer_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    dft_storage_buffer_descriptor_set_layout_binding.descriptorCount = 1;
    dft_storage_buffer_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    dft_storage_buffer_descriptor_set_layout_binding.pImmutableSamplers = NULL;
    VkDescriptorSetLayoutCreateInfo dft_storage_buffer_descriptor_set_layout_info;
    dft_storage_buffer_descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dft_storage_buffer_descriptor_set_layout_info.pNext = NULL;
    dft_storage_buffer_descriptor_set_layout_info.flags = 0;
    dft_storage_buffer_descriptor_set_layout_info.bindingCount = 1;
    dft_storage_buffer_descriptor_set_layout_info.pBindings = &dft_storage_buffer_descriptor_set_layout_binding;
    VK_CHECK_RES(vkCreateDescriptorSetLayout(vulkan->device, &dft_storage_buffer_descriptor_set_layout_info, NULL, &dft_storage_buffer_descriptor_set_layout));
    VkDescriptorSetLayout dft_storage_buffer_descriptor_set_layouts[VULKAN_MAX_FRAMES_IN_FLIGHT] = { dft_storage_buffer_descriptor_set_layout, dft_storage_buffer_descriptor_set_layout };
    VkDescriptorSetAllocateInfo dft_storage_buffer_descriptor_set_info;
    dft_storage_buffer_descriptor_set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dft_storage_buffer_descriptor_set_info.pNext = NULL;
    dft_storage_buffer_descriptor_set_info.descriptorPool = descriptor_pool;
    dft_storage_buffer_descriptor_set_info.descriptorSetCount = VULKAN_MAX_FRAMES_IN_FLIGHT;
    dft_storage_buffer_descriptor_set_info.pSetLayouts = dft_storage_buffer_descriptor_set_layouts;
    VK_CHECK_RES(vkAllocateDescriptorSets(vulkan->device, &dft_storage_buffer_descriptor_set_info, dft_storage_buffer_descriptor_sets));
    VkDescriptorBufferInfo dft_storage_buffer_descriptor_set_infos[VULKAN_MAX_FRAMES_IN_FLIGHT];
    VkWriteDescriptorSet dft_storage_buffer_descriptor_set_writes[VULKAN_MAX_FRAMES_IN_FLIGHT];
    for (uint32_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        dft_storage_buffer_descriptor_set_infos[i].buffer = dft_storage_buffers[i];
        dft_storage_buffer_descriptor_set_infos[i].offset = 0;
        dft_storage_buffer_descriptor_set_infos[i].range = VK_WHOLE_SIZE;
        dft_storage_buffer_descriptor_set_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        dft_storage_buffer_descriptor_set_writes[i].pNext = NULL;
        dft_storage_buffer_descriptor_set_writes[i].dstSet = dft_storage_buffer_descriptor_sets[i];
        dft_storage_buffer_descriptor_set_writes[i].dstBinding = 0;
        dft_storage_buffer_descriptor_set_writes[i].dstArrayElement = 0;
        dft_storage_buffer_descriptor_set_writes[i].descriptorCount = 1;
        dft_storage_buffer_descriptor_set_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        dft_storage_buffer_descriptor_set_writes[i].pImageInfo = NULL;
        dft_storage_buffer_descriptor_set_writes[i].pBufferInfo = &dft_storage_buffer_descriptor_set_infos[i];
        dft_storage_buffer_descriptor_set_writes[i].pTexelBufferView = NULL;
    }
    vkUpdateDescriptorSets(vulkan->device, VULKAN_MAX_FRAMES_IN_FLIGHT, dft_storage_buffer_descriptor_set_writes, 0, NULL);

    // Fullscreen graphics pipeline
    VulkanCreateShader(vulkan, "data/shaders/scene_columns.vert.spv", &fullscreen_vertex_shader, "SceneColumns: Vertex Shader");
    VulkanCreateShader(vulkan, "data/shaders/scene_columns.frag.spv", &fullscreen_fragment_shader, "SceneColumns: Fragment Shader");
    VkPipelineShaderStageCreateInfo fullscreen_shader_infos[2];
    fullscreen_shader_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fullscreen_shader_infos[0].pNext = NULL;
    fullscreen_shader_infos[0].flags = 0;
    fullscreen_shader_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    fullscreen_shader_infos[0].module = fullscreen_vertex_shader;
    fullscreen_shader_infos[0].pName = "main";
    fullscreen_shader_infos[0].pSpecializationInfo = NULL;
    fullscreen_shader_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fullscreen_shader_infos[1].pNext = NULL;
    fullscreen_shader_infos[1].flags = 0;
    fullscreen_shader_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fullscreen_shader_infos[1].module = fullscreen_fragment_shader;
    fullscreen_shader_infos[1].pName = "main";
    fullscreen_shader_infos[1].pSpecializationInfo = NULL;
    VkVertexInputBindingDescription fullscreen_vertex_binding_0;
    fullscreen_vertex_binding_0.binding = 0;
    fullscreen_vertex_binding_0.stride = 4 * sizeof(float);
    fullscreen_vertex_binding_0.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription fullscreen_vertex_attributes[2];
    // Position
    fullscreen_vertex_attributes[0].location = 0;
    fullscreen_vertex_attributes[0].binding = 0;
    fullscreen_vertex_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
    fullscreen_vertex_attributes[0].offset = 0;
    // UV
    fullscreen_vertex_attributes[1].location = 1;
    fullscreen_vertex_attributes[1].binding = 0;
    fullscreen_vertex_attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    fullscreen_vertex_attributes[1].offset = 2 * sizeof(float);
    VkPipelineVertexInputStateCreateInfo fullscreen_graphics_pipeline_vertex_input_info;
    fullscreen_graphics_pipeline_vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    fullscreen_graphics_pipeline_vertex_input_info.pNext = NULL;
    fullscreen_graphics_pipeline_vertex_input_info.flags = 0;
    fullscreen_graphics_pipeline_vertex_input_info.vertexBindingDescriptionCount = 1;
    fullscreen_graphics_pipeline_vertex_input_info.pVertexBindingDescriptions = &fullscreen_vertex_binding_0;
    fullscreen_graphics_pipeline_vertex_input_info.vertexAttributeDescriptionCount = 2;
    fullscreen_graphics_pipeline_vertex_input_info.pVertexAttributeDescriptions = fullscreen_vertex_attributes;
    VkPipelineInputAssemblyStateCreateInfo fullscreen_graphics_pipeline_input_assembly_info;
    fullscreen_graphics_pipeline_input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    fullscreen_graphics_pipeline_input_assembly_info.pNext = NULL;
    fullscreen_graphics_pipeline_input_assembly_info.flags = 0;
    fullscreen_graphics_pipeline_input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    fullscreen_graphics_pipeline_input_assembly_info.primitiveRestartEnable = VK_FALSE;
    VkViewport fullscreen_viewport;
    fullscreen_viewport.x = 0.0f;
    fullscreen_viewport.y = 0.0f;
    fullscreen_viewport.width = (float)vulkan->surface_caps.currentExtent.width;
    fullscreen_viewport.height = (float)vulkan->surface_caps.currentExtent.height;
    fullscreen_viewport.minDepth = 0.0f;
    fullscreen_viewport.maxDepth = 1.0f;
    VkRect2D fullscreen_scissor;
    fullscreen_scissor.offset.x = (uint32_t)fullscreen_viewport.x;
    fullscreen_scissor.offset.y = (uint32_t)fullscreen_viewport.y;
    fullscreen_scissor.extent.width = (uint32_t)fullscreen_viewport.width;
    fullscreen_scissor.extent.height = (uint32_t)fullscreen_viewport.height;
    VkPipelineViewportStateCreateInfo fullscreen_graphics_pipeline_viewport_info;
    fullscreen_graphics_pipeline_viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    fullscreen_graphics_pipeline_viewport_info.pNext = NULL;
    fullscreen_graphics_pipeline_viewport_info.flags = 0;
    fullscreen_graphics_pipeline_viewport_info.viewportCount = 1;
    fullscreen_graphics_pipeline_viewport_info.pViewports = &fullscreen_viewport;
    fullscreen_graphics_pipeline_viewport_info.scissorCount = 1;
    fullscreen_graphics_pipeline_viewport_info.pScissors = &fullscreen_scissor;
    VkPipelineRasterizationStateCreateInfo fullscreen_graphics_pipeline_rasterization_info;
    fullscreen_graphics_pipeline_rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    fullscreen_graphics_pipeline_rasterization_info.pNext = NULL;
    fullscreen_graphics_pipeline_rasterization_info.flags = 0;
    fullscreen_graphics_pipeline_rasterization_info.depthClampEnable = VK_FALSE;
    fullscreen_graphics_pipeline_rasterization_info.rasterizerDiscardEnable = VK_FALSE;
    fullscreen_graphics_pipeline_rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
    fullscreen_graphics_pipeline_rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
    fullscreen_graphics_pipeline_rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    fullscreen_graphics_pipeline_rasterization_info.depthBiasEnable = VK_FALSE;
    fullscreen_graphics_pipeline_rasterization_info.depthBiasConstantFactor = 0.0f;
    fullscreen_graphics_pipeline_rasterization_info.depthBiasClamp = 0.0f;
    fullscreen_graphics_pipeline_rasterization_info.depthBiasSlopeFactor = 0.0f;
    fullscreen_graphics_pipeline_rasterization_info.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo fullscreen_graphics_pipeline_multisample_info;
    fullscreen_graphics_pipeline_multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    fullscreen_graphics_pipeline_multisample_info.pNext = NULL;
    fullscreen_graphics_pipeline_multisample_info.flags = 0;
    fullscreen_graphics_pipeline_multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    fullscreen_graphics_pipeline_multisample_info.sampleShadingEnable = VK_FALSE;
    fullscreen_graphics_pipeline_multisample_info.pSampleMask = NULL;
    fullscreen_graphics_pipeline_multisample_info.alphaToCoverageEnable = VK_FALSE;
    fullscreen_graphics_pipeline_multisample_info.alphaToOneEnable = VK_FALSE;
    VkPipelineDepthStencilStateCreateInfo fullscreen_graphics_pipeline_depth_info;
    fullscreen_graphics_pipeline_depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    fullscreen_graphics_pipeline_depth_info.pNext = NULL;
    fullscreen_graphics_pipeline_depth_info.flags = 0;
    fullscreen_graphics_pipeline_depth_info.depthTestEnable = VK_TRUE;
    fullscreen_graphics_pipeline_depth_info.depthWriteEnable = VK_TRUE;
    fullscreen_graphics_pipeline_depth_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    fullscreen_graphics_pipeline_depth_info.depthBoundsTestEnable = VK_FALSE;
    fullscreen_graphics_pipeline_depth_info.stencilTestEnable = VK_FALSE;
    fullscreen_graphics_pipeline_depth_info.front.failOp = VK_STENCIL_OP_KEEP;
    fullscreen_graphics_pipeline_depth_info.front.passOp = VK_STENCIL_OP_KEEP;
    fullscreen_graphics_pipeline_depth_info.front.depthFailOp = VK_STENCIL_OP_KEEP;
    fullscreen_graphics_pipeline_depth_info.front.compareOp = VK_COMPARE_OP_NEVER;
    fullscreen_graphics_pipeline_depth_info.front.compareMask = 0x0;
    fullscreen_graphics_pipeline_depth_info.front.writeMask = 0x0;
    fullscreen_graphics_pipeline_depth_info.front.reference = 0x0;
    fullscreen_graphics_pipeline_depth_info.back.failOp = VK_STENCIL_OP_KEEP;
    fullscreen_graphics_pipeline_depth_info.back.passOp = VK_STENCIL_OP_KEEP;
    fullscreen_graphics_pipeline_depth_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
    fullscreen_graphics_pipeline_depth_info.back.compareOp = VK_COMPARE_OP_NEVER;
    fullscreen_graphics_pipeline_depth_info.back.compareMask = 0x0;
    fullscreen_graphics_pipeline_depth_info.back.writeMask = 0x0;
    fullscreen_graphics_pipeline_depth_info.back.reference = 0x0;
    fullscreen_graphics_pipeline_depth_info.minDepthBounds = 0.0f;
    fullscreen_graphics_pipeline_depth_info.maxDepthBounds = 1.0f;
    VkPipelineColorBlendAttachmentState fullscreen_color_blend_attachment_0;
    fullscreen_color_blend_attachment_0.blendEnable = VK_FALSE;
    fullscreen_color_blend_attachment_0.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    fullscreen_color_blend_attachment_0.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    fullscreen_color_blend_attachment_0.colorBlendOp = VK_BLEND_OP_ADD;
    fullscreen_color_blend_attachment_0.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    fullscreen_color_blend_attachment_0.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    fullscreen_color_blend_attachment_0.alphaBlendOp = VK_BLEND_OP_ADD;
    fullscreen_color_blend_attachment_0.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo fullscreen_graphics_pipeline_color_blend_info;
    fullscreen_graphics_pipeline_color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    fullscreen_graphics_pipeline_color_blend_info.pNext = NULL;
    fullscreen_graphics_pipeline_color_blend_info.flags = 0;
    fullscreen_graphics_pipeline_color_blend_info.logicOpEnable = VK_FALSE;
    fullscreen_graphics_pipeline_color_blend_info.attachmentCount = 1;
    fullscreen_graphics_pipeline_color_blend_info.pAttachments = &fullscreen_color_blend_attachment_0;
    fullscreen_graphics_pipeline_color_blend_info.blendConstants[0] = 1.0f;
    fullscreen_graphics_pipeline_color_blend_info.blendConstants[1] = 1.0f;
    fullscreen_graphics_pipeline_color_blend_info.blendConstants[2] = 1.0f;
    fullscreen_graphics_pipeline_color_blend_info.blendConstants[3] = 1.0f;
    VkPipelineDynamicStateCreateInfo fullscreen_graphics_pipeline_dynamic_info;
    fullscreen_graphics_pipeline_dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    fullscreen_graphics_pipeline_dynamic_info.pNext = NULL;
    fullscreen_graphics_pipeline_dynamic_info.flags = 0;
    fullscreen_graphics_pipeline_dynamic_info.dynamicStateCount = 0;
    fullscreen_graphics_pipeline_dynamic_info.pDynamicStates = NULL;
    VkPushConstantRange fullscreen_graphics_push_constant_range;
    fullscreen_graphics_push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    fullscreen_graphics_push_constant_range.size = 2 * sizeof(float); // vec2 resolution
    fullscreen_graphics_push_constant_range.offset = 0;
    VkPipelineLayoutCreateInfo fullscreen_graphics_pipeline_layout_info;
    fullscreen_graphics_pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    fullscreen_graphics_pipeline_layout_info.pNext = NULL;
    fullscreen_graphics_pipeline_layout_info.flags = 0;
    fullscreen_graphics_pipeline_layout_info.setLayoutCount = 1;
    fullscreen_graphics_pipeline_layout_info.pSetLayouts = &dft_storage_buffer_descriptor_set_layout;
    fullscreen_graphics_pipeline_layout_info.pushConstantRangeCount = 1;
    fullscreen_graphics_pipeline_layout_info.pPushConstantRanges = &fullscreen_graphics_push_constant_range;
    VK_CHECK_RES(vkCreatePipelineLayout(vulkan->device, &fullscreen_graphics_pipeline_layout_info, NULL, &fullscreen_graphics_pipeline_layout));
    VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)fullscreen_graphics_pipeline_layout, "Fullscreen Graphics Pipeline Layout (Main Render Pass)");
    VkGraphicsPipelineCreateInfo fullscreen_graphics_pipeline_info;
    fullscreen_graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    fullscreen_graphics_pipeline_info.pNext = &pipeline_rendering_info;
    fullscreen_graphics_pipeline_info.flags = 0;
    fullscreen_graphics_pipeline_info.stageCount = 2;
    fullscreen_graphics_pipeline_info.pStages = fullscreen_shader_infos;
    fullscreen_graphics_pipeline_info.pVertexInputState = &fullscreen_graphics_pipeline_vertex_input_info;
    fullscreen_graphics_pipeline_info.pInputAssemblyState = &fullscreen_graphics_pipeline_input_assembly_info;
    fullscreen_graphics_pipeline_info.pTessellationState = NULL;
    fullscreen_graphics_pipeline_info.pViewportState = &fullscreen_graphics_pipeline_viewport_info;
    fullscreen_graphics_pipeline_info.pRasterizationState = &fullscreen_graphics_pipeline_rasterization_info;
    fullscreen_graphics_pipeline_info.pMultisampleState = &fullscreen_graphics_pipeline_multisample_info;
    fullscreen_graphics_pipeline_info.pDepthStencilState = &fullscreen_graphics_pipeline_depth_info;
    fullscreen_graphics_pipeline_info.pColorBlendState = &fullscreen_graphics_pipeline_color_blend_info;
    fullscreen_graphics_pipeline_info.pDynamicState = &fullscreen_graphics_pipeline_dynamic_info;
    fullscreen_graphics_pipeline_info.layout = fullscreen_graphics_pipeline_layout;
    fullscreen_graphics_pipeline_info.renderPass = VK_NULL_HANDLE;
    fullscreen_graphics_pipeline_info.subpass = 0;
    fullscreen_graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    fullscreen_graphics_pipeline_info.basePipelineIndex = -1;
    VK_CHECK_RES(vkCreateGraphicsPipelines(vulkan->device, vulkan->pipeline_cache, 1, &fullscreen_graphics_pipeline_info, NULL, &fullscreen_graphics_pipeline));
    VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_PIPELINE, (uint64_t)fullscreen_graphics_pipeline, "Fullscreen Graphics Pipeline (Main Render Pass)");
    
    resolution[0] = (float)vulkan->surface_caps.currentExtent.width;
    resolution[1] = (float)vulkan->surface_caps.currentExtent.height;
}

void SceneColumnsRecreateFramebuffers(vulkan_context_t* vulkan)
{   
    resolution[0] = (float)vulkan->surface_caps.currentExtent.width;
    resolution[1] = (float)vulkan->surface_caps.currentExtent.height;
}

void SceneColumnsRender(vulkan_context_t* vulkan, VkCommandBuffer frame_command_buffer, uint32_t frame_image_index, uint32_t frame_resource_index)
{
    // Color attachment
    VkRenderingAttachmentInfo color_attachment;
    color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment.pNext = NULL;
    color_attachment.imageView = vulkan->intermediate_swapchain_image_view;
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment.resolveMode = VK_RESOLVE_MODE_NONE;
    color_attachment.resolveImageView = VK_NULL_HANDLE;
    color_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue.color.float32[0] = 0.0f;
    color_attachment.clearValue.color.float32[1] = 0.0f;
    color_attachment.clearValue.color.float32[2] = 0.0f;
    color_attachment.clearValue.color.float32[3] = 1.0f;
    // Depth attachment
    VkRenderingAttachmentInfo depth_attachment;
    depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depth_attachment.pNext = NULL;
    depth_attachment.imageView = vulkan->depth_stencil_image_view;
    depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.resolveMode = VK_RESOLVE_MODE_NONE;
    depth_attachment.resolveImageView = VK_NULL_HANDLE;
    depth_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.clearValue.depthStencil.depth = 1.0f;
    depth_attachment.clearValue.depthStencil.stencil = 0x0;
    // Stencil attachment
    VkRenderingAttachmentInfo stencil_attachment;
    stencil_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    stencil_attachment.pNext = NULL;
    stencil_attachment.imageView = vulkan->depth_stencil_image_view;
    stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    stencil_attachment.resolveMode = VK_RESOLVE_MODE_NONE;
    stencil_attachment.resolveImageView = VK_NULL_HANDLE;
    stencil_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    stencil_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    stencil_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    stencil_attachment.clearValue.depthStencil.depth = 1.0f;
    stencil_attachment.clearValue.depthStencil.stencil = 0x0;

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
    rendering_info.pDepthAttachment = &depth_attachment;
    rendering_info.pStencilAttachment = &stencil_attachment;

    // Main render pass
    VulkanCmdBeginDebugUtilsLabel(vulkan, frame_command_buffer, "SceneColumns: Main Render Pass");
    vkCmdBeginRendering(frame_command_buffer, &rendering_info);
    vkCmdBindPipeline(frame_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fullscreen_graphics_pipeline);
    vkCmdBindDescriptorSets(frame_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fullscreen_graphics_pipeline_layout, 0, 1, &dft_storage_buffer_descriptor_sets[frame_resource_index], 0, NULL);
    vkCmdPushConstants(frame_command_buffer, fullscreen_graphics_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 2 * sizeof(float), &resolution);
    VkDeviceSize fullscreen_vertex_buffer_offset = 0;
    vkCmdBindVertexBuffers(frame_command_buffer, 0, 1, &fullscreen_vertex_buffer, &fullscreen_vertex_buffer_offset);
    vkCmdDraw(frame_command_buffer, 6, 1, 0, 0);
    vkCmdEndRendering(frame_command_buffer);
    VulkanCmdEndDebugUtilsLabel(vulkan, frame_command_buffer);
}

void SceneColumnsDestroy(vulkan_context_t* vulkan)
{
    vkDestroyPipeline(vulkan->device, fullscreen_graphics_pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, fullscreen_graphics_pipeline_layout, NULL);
    vkDestroyShaderModule(vulkan->device, fullscreen_fragment_shader, NULL);
    vkDestroyShaderModule(vulkan->device, fullscreen_vertex_shader, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, dft_storage_buffer_descriptor_set_layout, NULL);
    vkDestroyDescriptorPool(vulkan->device, descriptor_pool, NULL);
    VulkanDestroyBuffer(vulkan, &fullscreen_vertex_buffer, &fullscreen_vertex_buffer_memory);
}