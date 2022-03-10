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
static VkRenderPass render_pass;

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

// Framebuffer
static VkFramebuffer framebuffer;
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
    VkAttachmentDescription render_pass_output_attachment;
    render_pass_output_attachment.flags = 0;
    render_pass_output_attachment.format = vulkan->intermediate_swapchain_image_format;
    render_pass_output_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    render_pass_output_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    render_pass_output_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    render_pass_output_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    render_pass_output_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    render_pass_output_attachment.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    render_pass_output_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentDescription render_pass_depth_stencil_attachment;
    render_pass_depth_stencil_attachment.flags = 0;
    render_pass_depth_stencil_attachment.format = VK_FORMAT_D24_UNORM_S8_UINT;
    render_pass_depth_stencil_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    render_pass_depth_stencil_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    render_pass_depth_stencil_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    render_pass_depth_stencil_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    render_pass_depth_stencil_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    render_pass_depth_stencil_attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    render_pass_depth_stencil_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentDescription render_pass_attachments[2] = {
        render_pass_output_attachment,
        render_pass_depth_stencil_attachment
    };
    VkAttachmentReference render_pass_subpass_output_attachment;
    render_pass_subpass_output_attachment.attachment = 0;
    render_pass_subpass_output_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference render_pass_subpass_depth_stencil_attachment;
    render_pass_subpass_depth_stencil_attachment.attachment = 1;
    render_pass_subpass_depth_stencil_attachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkSubpassDescription render_pass_subpass;
    render_pass_subpass.flags = 0;
    render_pass_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    render_pass_subpass.inputAttachmentCount = 0;
    render_pass_subpass.pInputAttachments = NULL;
    render_pass_subpass.colorAttachmentCount = 1;
    render_pass_subpass.pColorAttachments = &render_pass_subpass_output_attachment;
    render_pass_subpass.pResolveAttachments = NULL;
    render_pass_subpass.pDepthStencilAttachment = &render_pass_subpass_depth_stencil_attachment;
    render_pass_subpass.preserveAttachmentCount = 0;
    render_pass_subpass.pPreserveAttachments = NULL;
    VkSubpassDependency render_pass_dependency;
    render_pass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    render_pass_dependency.dstSubpass = 0;
    render_pass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    render_pass_dependency.srcAccessMask = 0;
    render_pass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    render_pass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    render_pass_dependency.dependencyFlags = 0;
    VkRenderPassCreateInfo render_pass_info;
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pNext = NULL;
    render_pass_info.flags = 0;
    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = render_pass_attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &render_pass_subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &render_pass_dependency;
    VK_CHECK_RES(vkCreateRenderPass(vulkan->device, &render_pass_info, NULL, &render_pass));
    VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)render_pass, "SceneColumns: Main Render Pass");

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
    fullscreen_graphics_pipeline_info.pNext = NULL;
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
    fullscreen_graphics_pipeline_info.renderPass = render_pass;
    fullscreen_graphics_pipeline_info.subpass = 0;
    fullscreen_graphics_pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    fullscreen_graphics_pipeline_info.basePipelineIndex = -1;
    VK_CHECK_RES(vkCreateGraphicsPipelines(vulkan->device, vulkan->pipeline_cache, 1, &fullscreen_graphics_pipeline_info, NULL, &fullscreen_graphics_pipeline));
    VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_PIPELINE, (uint64_t)fullscreen_graphics_pipeline, "Fullscreen Graphics Pipeline (Main Render Pass)");

    // Framebuffer
    VkImageView attachments[2] = {
        vulkan->intermediate_swapchain_image_view,
        vulkan->depth_stencil_image_view
    };
    VkFramebufferCreateInfo framebuffer_info;
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.pNext = NULL;
    framebuffer_info.flags = 0;
    framebuffer_info.renderPass = render_pass;
    framebuffer_info.attachmentCount = 2;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = vulkan->surface_caps.currentExtent.width;
    framebuffer_info.height = vulkan->surface_caps.currentExtent.height;
    framebuffer_info.layers = 1;
    VK_CHECK_RES(vkCreateFramebuffer(vulkan->device, &framebuffer_info, NULL, &framebuffer));
    
    memset(vulkan->vulkan_object_name, 0, 128);
    sprintf(vulkan->vulkan_object_name, "SceneColumns: Main Framebuffer");
    VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)framebuffer, vulkan->vulkan_object_name);
    
    resolution[0] = (float)vulkan->surface_caps.currentExtent.width;
    resolution[1] = (float)vulkan->surface_caps.currentExtent.height;
}

void SceneColumnsRecreateFramebuffers(vulkan_context_t* vulkan)
{
    // Delete old framebuffer
    vkDestroyFramebuffer(vulkan->device, framebuffer, NULL);

    // Create new framebuffer
    VkImageView attachments[2] = {
        vulkan->intermediate_swapchain_image_view,
        vulkan->depth_stencil_image_view
    };
    VkFramebufferCreateInfo framebuffer_info;
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.pNext = NULL;
    framebuffer_info.flags = 0;
    framebuffer_info.renderPass = render_pass;
    framebuffer_info.attachmentCount = 2;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = vulkan->surface_caps.currentExtent.width;
    framebuffer_info.height = vulkan->surface_caps.currentExtent.height;
    framebuffer_info.layers = 1;
    VK_CHECK_RES(vkCreateFramebuffer(vulkan->device, &framebuffer_info, NULL, &framebuffer));
    
    memset(vulkan->vulkan_object_name, 0, 128);
    sprintf(vulkan->vulkan_object_name, "SceneColumns: Main Framebuffer");
    VulkanSetObjectName(vulkan, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)framebuffer, vulkan->vulkan_object_name);
    
    resolution[0] = (float)vulkan->surface_caps.currentExtent.width;
    resolution[1] = (float)vulkan->surface_caps.currentExtent.height;
}

void SceneColumnsRender(vulkan_context_t* vulkan, VkCommandBuffer frame_command_buffer, uint32_t frame_image_index, uint32_t frame_resource_index)
{
    // Main render pass
    VulkanCmdBeginDebugUtilsLabel(vulkan, frame_command_buffer, "SceneColumns: Main Render Pass");
    VkRenderPassBeginInfo frame_render_pass_info;
    frame_render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    frame_render_pass_info.pNext = NULL;
    frame_render_pass_info.renderPass = render_pass;
    frame_render_pass_info.framebuffer = framebuffer;
    frame_render_pass_info.renderArea.extent = vulkan->surface_caps.currentExtent;
    frame_render_pass_info.renderArea.offset.x = 0;
    frame_render_pass_info.renderArea.offset.y = 0;
    frame_render_pass_info.clearValueCount = 2;
    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.0;
    clear_values[0].color.float32[1] = 0.0;
    clear_values[0].color.float32[2] = 0.0;
    clear_values[0].color.float32[3] = 1.0f;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;
    frame_render_pass_info.pClearValues = clear_values;
    vkCmdBeginRenderPass(frame_command_buffer, &frame_render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(frame_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fullscreen_graphics_pipeline);
    vkCmdBindDescriptorSets(frame_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, fullscreen_graphics_pipeline_layout, 0, 1, &dft_storage_buffer_descriptor_sets[frame_resource_index], 0, NULL);
    vkCmdPushConstants(frame_command_buffer, fullscreen_graphics_pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 2 * sizeof(float), &resolution);
    VkDeviceSize fullscreen_vertex_buffer_offset = 0;
    vkCmdBindVertexBuffers(frame_command_buffer, 0, 1, &fullscreen_vertex_buffer, &fullscreen_vertex_buffer_offset);
    vkCmdDraw(frame_command_buffer, 6, 1, 0, 0);
    vkCmdEndRenderPass(frame_command_buffer);
    VulkanCmdEndDebugUtilsLabel(vulkan, frame_command_buffer);
}

void SceneColumnsDestroy(vulkan_context_t* vulkan)
{
    vkDestroyFramebuffer(vulkan->device, framebuffer, NULL);
    vkDestroyPipeline(vulkan->device, fullscreen_graphics_pipeline, NULL);
    vkDestroyPipelineLayout(vulkan->device, fullscreen_graphics_pipeline_layout, NULL);
    vkDestroyShaderModule(vulkan->device, fullscreen_fragment_shader, NULL);
    vkDestroyShaderModule(vulkan->device, fullscreen_vertex_shader, NULL);
    vkDestroyDescriptorSetLayout(vulkan->device, dft_storage_buffer_descriptor_set_layout, NULL);
    vkDestroyDescriptorPool(vulkan->device, descriptor_pool, NULL);
    vkDestroyRenderPass(vulkan->device, render_pass, NULL);
    VulkanDestroyBuffer(vulkan, &fullscreen_vertex_buffer, &fullscreen_vertex_buffer_memory);
}