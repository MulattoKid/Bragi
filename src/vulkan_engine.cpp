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

#include "vulkan_engine.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

extern bool running;

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugMessagePrinter(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    printf("Vulkan validation message: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

void VulkanInit(HINSTANCE win_instance, HWND win_window, vulkan_context_t* vulkan)
{
    char object_name[128];

    printf("Vulkan:\n");

    const uint32_t target_api_version = VK_MAKE_API_VERSION(0, 1, 0, 0);

    // Determine instance version
    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
    uint32_t instance_api_version = 0;
    PFN_vkEnumerateInstanceVersion enumerateInstanceVersionPFN = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");
    if (enumerateInstanceVersionPFN == NULL) // If the vkGetInstanceProcAddr returns NULL for vkEnumerateInstanceVersion, it is a Vulkan 1.0 implementation
    {
        instance_api_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
    }
    else // Otherwise, the application can call vkEnumerateInstanceVersion to determine the version of Vulkan
    {
        enumerateInstanceVersionPFN(&instance_api_version);
    }
    if (instance_api_version < target_api_version)
    {
        printf("Vulkan instance's API version (%u.%u.%u) is less than the minimum version required (%u.%u.%u)\n",
               VK_VERSION_MAJOR(instance_api_version), VK_VERSION_MINOR(instance_api_version), VK_VERSION_PATCH(instance_api_version),
               VK_VERSION_MAJOR(target_api_version), VK_VERSION_MINOR(target_api_version), VK_VERSION_PATCH(target_api_version));
        exit(EXIT_FAILURE);
    }
    printf("\tVulkan instance's API version (%u.%u.%u)\n",
           VK_VERSION_MAJOR(instance_api_version), VK_VERSION_MINOR(instance_api_version), VK_VERSION_PATCH(instance_api_version));

    // App info
    VkApplicationInfo app_info;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = "SoundPlayer";
    app_info.applicationVersion = 1;
    app_info.pEngineName = "SoundPlayerVulkanEngine";
    app_info.engineVersion = 1;
    app_info.apiVersion = target_api_version;

    // Instance layers
    const uint32_t instance_layers_required_count = 1;
    const char* instance_layers_required[instance_layers_required_count] = {
        "VK_LAYER_KHRONOS_validation"
    };
    uint32_t instance_layer_count = 0;
    VK_CHECK_RES(vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL));
    VkLayerProperties* instance_layers = (VkLayerProperties*)malloc(instance_layer_count * sizeof(VkLayerProperties));
    VK_CHECK_RES(vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers));
    printf("\tInstance layers:\n");
    uint32_t instance_layers_required_count_remaining = instance_layers_required_count;
    for (uint32_t i = 0; i < instance_layer_count; i++)
    {
        for (uint32_t j = 0; j < instance_layers_required_count; j++)
        {
            if (strcmp(instance_layers_required[j], instance_layers[i].layerName) == 0)
            {
                instance_layers_required_count_remaining--;
            }
        }
        printf("\t\t%s\n", instance_layers[i].layerName);
    }
    assert(instance_layers_required_count_remaining == 0);
    free(instance_layers);

    // Instance extensions
    const uint32_t instance_extensions_required_count = 3;
    const char* instance_extensions_required[instance_extensions_required_count] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    uint32_t instance_extension_count = 0;
    VK_CHECK_RES(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL));
    VkExtensionProperties* instance_extensions = (VkExtensionProperties*)malloc(instance_extension_count * sizeof(VkExtensionProperties));
    VK_CHECK_RES(vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions));
    printf("\tInstance extensions:\n");
    uint32_t instance_extensions_required_count_remaining = instance_extensions_required_count;
    for (uint32_t i = 0; i < instance_extension_count; i++)
    {
        for (uint32_t j = 0; j < instance_extensions_required_count; j++)
        {
            if (strcmp(instance_extensions_required[j], instance_extensions[i].extensionName) == 0)
            {
                instance_extensions_required_count_remaining--;
            }
        }
        printf("\t\t%s\n", instance_extensions[i].extensionName);
    }
    // When running through RenderDoc 'VK_EXT_DEBUG_UTILS_EXTENSION_NAME' is reported twice, so the count wraps around
    assert((instance_extensions_required_count_remaining == 0) || (instance_extensions_required_count_remaining > instance_extensions_required_count));
    free(instance_extensions);

    // Instance info
    VkInstanceCreateInfo instance_info;
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = NULL;
    instance_info.flags = 0;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = instance_layers_required_count;
    instance_info.ppEnabledLayerNames = instance_layers_required;
    instance_info.enabledExtensionCount = instance_extensions_required_count;
    instance_info.ppEnabledExtensionNames = instance_extensions_required;
    VkInstance instance = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateInstance(&instance_info, NULL, &instance));

    // Instance debug messenger
    VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_info;
    debug_utils_messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_utils_messenger_info.pNext = NULL;
    debug_utils_messenger_info.flags = 0;
    debug_utils_messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_utils_messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_utils_messenger_info.pfnUserCallback = VulkanDebugMessagePrinter;
    debug_utils_messenger_info.pUserData = NULL;
    VkDebugUtilsMessengerEXT debug_messenger;
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    assert(vkCreateDebugUtilsMessengerEXT != NULL);
    VK_CHECK_RES(vkCreateDebugUtilsMessengerEXT(instance, &debug_utils_messenger_info, NULL, &debug_messenger));
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
    assert(vkSetDebugUtilsObjectNameEXT != NULL);
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
    assert(vkCmdBeginDebugUtilsLabelEXT != NULL);
    PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
    assert(vkCmdEndDebugUtilsLabelEXT != NULL);

    // Physical device
    // TODO (Daniel): currently picks last discrete GPU
    uint32_t physical_device_count = 0;
    VK_CHECK_RES(vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL));
    VkPhysicalDevice* physical_devices = (VkPhysicalDevice*)malloc(physical_device_count * sizeof(VkPhysicalDevice));
    VK_CHECK_RES(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices));
    printf("\tPhysical devices:\n");
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physical_device_props;
    for (uint32_t i = 0; i < physical_device_count; i++)
    {
        vkGetPhysicalDeviceProperties(physical_devices[i], &physical_device_props);
        printf("\t\t%s\n", physical_device_props.deviceName);
        printf("\t\t\tVendor ID: %u\n", physical_device_props.vendorID);
        printf("\t\t\tDevice ID: %u\n", physical_device_props.deviceID);
        printf("\t\t\tDriver version: %u\n", physical_device_props.driverVersion);
        printf("\t\t\tAPI version: %u.%u.%u\n", VK_API_VERSION_MAJOR(physical_device_props.apiVersion), VK_API_VERSION_MINOR(physical_device_props.apiVersion), VK_API_VERSION_PATCH(physical_device_props.apiVersion));
        if (physical_device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            physical_device = physical_devices[i];
        }
    }
    assert(physical_device != VK_NULL_HANDLE);
    if (physical_device_props.apiVersion < target_api_version)
    {
        printf("Physical device's (%s) API version (%u.%u.%u) is less than the minimum version required (%u.%u.%u)\n",
               physical_device_props.deviceName,
               VK_VERSION_MAJOR(physical_device_props.apiVersion), VK_VERSION_MINOR(physical_device_props.apiVersion), VK_VERSION_PATCH(physical_device_props.apiVersion),
               VK_VERSION_MAJOR(target_api_version), VK_VERSION_MINOR(target_api_version), VK_VERSION_PATCH(target_api_version));
        exit(EXIT_FAILURE);
    }
    free(physical_devices);

    // Physical device memory properties
    VkPhysicalDeviceMemoryProperties physical_device_memory_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_props);
    printf("\tMemory types:\n");
    for (uint32_t i = 0; i < physical_device_memory_props.memoryTypeCount; i++)
    {
        printf("\t\t%u:\n", i);
        if (physical_device_memory_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        {
            printf("\t\t\tVK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT\n");
        }
        if (physical_device_memory_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            printf("\t\t\tVK_MEMORY_PROPERTY_HOST_VISIBLE_BIT\n");
        }
        if (physical_device_memory_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        {
            printf("\t\t\tVK_MEMORY_PROPERTY_HOST_COHERENT_BIT\n");
        }
        if (physical_device_memory_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
        {
            printf("\t\t\tVK_MEMORY_PROPERTY_HOST_CACHED_BIT\n");
        }
        if (physical_device_memory_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
        {
            printf("\t\t\tVK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT\n");
        }
        printf("\t\t\tHeap: %u\n", physical_device_memory_props.memoryTypes[i].heapIndex);
    }
    printf("\tMemory heaps:\n");
    for (uint32_t i = 0; i < physical_device_memory_props.memoryHeapCount; i++)
    {
        printf("\t\t%u:\n", i);
        printf("\t\t\tSize: %llu\n", physical_device_memory_props.memoryHeaps[i].size);
        if (physical_device_memory_props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            printf("\t\t\tVK_MEMORY_HEAP_DEVICE_LOCAL_BIT\n");
        }
        if (physical_device_memory_props.memoryHeaps[i].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
        {
            printf("\t\t\tVK_MEMORY_HEAP_MULTI_INSTANCE_BIT\n");
        }
    }

    // Physical device formats
    // Only do non-extension formats in Vulkan 1.0
    VkFormatProperties* physical_device_format_props = (VkFormatProperties*)malloc((VK_FORMAT_ASTC_12x12_SRGB_BLOCK - VK_FORMAT_UNDEFINED) * sizeof(VkFormatProperties));
    for (uint32_t i = VK_FORMAT_UNDEFINED; i < VK_FORMAT_ASTC_12x12_SRGB_BLOCK; i++)
    {
        vkGetPhysicalDeviceFormatProperties(physical_device, (VkFormat)i, &physical_device_format_props[i]);
    }

    // Win32 surface
    VkWin32SurfaceCreateInfoKHR surface_info;
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.pNext = NULL;
    surface_info.flags = 0;
    surface_info.hinstance = win_instance;
    surface_info.hwnd = win_window;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateWin32SurfaceKHR(instance, &surface_info, NULL, &surface));
    DEVMODEA display_settings;
    EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &display_settings);
    DWORD display_frequency = display_settings.dmDisplayFrequency;
    float display_frame_time_us = 1000000.0f / (float)display_frequency;
    
    // Surface capabilities
    const VkImageUsageFlags surface_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    VkSurfaceCapabilitiesKHR surface_caps;
    VK_CHECK_RES(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_caps));
    assert(surface_caps.supportedUsageFlags & surface_usage);
    printf("\tSurface capabilities:\n"
           "\t\tMin image count: %u\n"
           "\t\tMax image count: %u\n"
           "\t\tMinimum extent: %ux%u\n"
           "\t\tMaximum extent: %ux%u\n"
           "\t\tCurrent extent: %ux%u\n",
           surface_caps.minImageCount,
           surface_caps.maxImageCount,
           surface_caps.minImageExtent.width, surface_caps.minImageExtent.height,
           surface_caps.maxImageExtent.width, surface_caps.maxImageExtent.height,
           surface_caps.currentExtent.width, surface_caps.currentExtent.height);

    // Surface format and color space
    // TODO (Daniel): currently only support combo (VK_FORMAT_B8G8R8A8_SRGB & VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    const VkFormat surface_format = VK_FORMAT_B8G8R8A8_SRGB;
    assert(physical_device_format_props[surface_format].optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
    assert(physical_device_format_props[surface_format].optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
    assert(physical_device_format_props[surface_format].optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);
    assert(physical_device_format_props[surface_format].optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT);
    assert(physical_device_format_props[surface_format].optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT);
    const VkColorSpaceKHR surface_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    uint32_t surface_format_count = 0;
    VK_CHECK_RES(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, NULL));
    VkSurfaceFormatKHR* surface_formats = (VkSurfaceFormatKHR*)malloc(surface_format_count * sizeof(VkSurfaceFormatKHR));
    VK_CHECK_RES(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats));
    printf("\tSurface formats:\n");
    uint32_t surface_format_required_count_remaining = 1;
    for (uint32_t i = 0; i < surface_format_count; i++)
    {
        if ((surface_formats[i].format == surface_format) &&
            (surface_formats[i].colorSpace == surface_color_space))
        {
            surface_format_required_count_remaining--;
        }
        printf("\t\tCombo %u:\n", i);
        printf("\t\t\tFormat: %i\n", surface_formats[i].format);
        printf("\t\t\tColor space: %i\n", surface_formats[i].colorSpace);
    }
    assert(surface_format_required_count_remaining == 0);
    free(surface_formats);

    // Surface present mode
    // TODO (Daniel): currently only supports VK_PRESENT_MODE_FIFO_KHR
    const VkPresentModeKHR surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t surface_present_mode_count = 0;
    VK_CHECK_RES(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &surface_present_mode_count, NULL));
    VkPresentModeKHR* surface_present_modes = (VkPresentModeKHR*)malloc(surface_present_mode_count * sizeof(VkPresentModeKHR));
    VK_CHECK_RES(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &surface_present_mode_count, surface_present_modes));
    printf("\tSurface present modes:\n");
    uint32_t surface_present_mode_count_remaining = 1;
    for (uint32_t i = 0; i < surface_present_mode_count; i++)
    {
        if (surface_present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            surface_present_mode_count_remaining--;
            printf("\t\tVK_PRESENT_MODE_IMMEDIATE_KHR\n");
        }
        else if (surface_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            printf("\t\tVK_PRESENT_MODE_MAILBOX_KHR\n");
        }
        else if (surface_present_modes[i] == VK_PRESENT_MODE_FIFO_KHR)
        {
            printf("\t\tVK_PRESENT_MODE_FIFO_KHR\n");
        }
        else if (surface_present_modes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
        {
            printf("\t\tVK_PRESENT_MODE_FIFO_RELAXED_KHR\n");
        }
    }
    assert(surface_present_mode_count_remaining == 0);
    free(surface_present_modes);

    // Queue index
    // TODO (Daniel): currently only supports one queue that supports both graphics and compute
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
    VkQueueFamilyProperties* queue_families = (VkQueueFamilyProperties*)malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);
    printf("\tQueues:\n");
    uint32_t queue_index = -1;
    for (uint32_t i = 0; i < queue_family_count; i++)
    {
        printf("\t\tQueue %u:\n", i);
        printf("\t\t\tFlags: ");
        if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
        {
            printf("VK_QUEUE_GRAPHICS_BIT ");
        }
        if ((queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT)
        {
            printf("VK_QUEUE_COMPUTE_BIT ");
        }
        if ((queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
        {
            printf("VK_QUEUE_TRANSFER_BIT ");
        }
        if ((queue_families[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) == VK_QUEUE_SPARSE_BINDING_BIT)
        {
            printf("VK_QUEUE_SPARSE_BINDING_BIT ");
        }
        printf("\n");
        printf("\t\t\tCount: %u\n", queue_families[i].queueCount);

        VkBool32 surface_present_support = VK_FALSE;
        VK_CHECK_RES(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &surface_present_support));

        if (((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT) &&
            ((queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT) &&
            (surface_present_support == VK_TRUE) &&
            (vkGetPhysicalDeviceWin32PresentationSupportKHR(physical_device, i) == VK_TRUE))
        {
            queue_index = i;
        }
    }
    assert(queue_index != (uint32_t)-1);
    free(queue_families);

    // Physcial device extensions
    const uint32_t physical_device_extensions_required_count = 1;
    const char* physical_device_extensions_required[physical_device_extensions_required_count] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    uint32_t physical_device_extension_count = 0;
    VK_CHECK_RES(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &physical_device_extension_count, NULL));
    VkExtensionProperties* physical_device_extensions = (VkExtensionProperties*)malloc(physical_device_extension_count * sizeof(VkExtensionProperties));
    VK_CHECK_RES(vkEnumerateDeviceExtensionProperties(physical_device, NULL, &physical_device_extension_count, physical_device_extensions));
    printf("\tPhysical device extensions:\n");
    uint32_t physical_device_extensions_required_remaining = physical_device_extensions_required_count;
    for (uint32_t i = 0; i < physical_device_extension_count; i++)
    {
        for (uint32_t j = 0; j < physical_device_extensions_required_count; j++)
        {
            if (strcmp(physical_device_extensions_required[j], physical_device_extensions[i].extensionName) == 0)
            {
                physical_device_extensions_required_remaining--;
            }
        }
        printf("\t\t%s\n", physical_device_extensions[i].extensionName);
    }
    assert(physical_device_extensions_required_remaining == 0);
    free(physical_device_extensions);

    // Device
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info;
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = NULL;
    queue_info.flags = 0;
    queue_info.queueFamilyIndex = queue_index;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;
    VkDeviceCreateInfo device_info;
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.flags = 0;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledLayerCount = 0;
    device_info.ppEnabledLayerNames = NULL;
    device_info.enabledExtensionCount = physical_device_extensions_required_count;
    device_info.ppEnabledExtensionNames = physical_device_extensions_required;
    device_info.pEnabledFeatures = NULL;
    VkDevice device = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateDevice(physical_device, &device_info, NULL, &device));

    // Queue
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queue_index, 0, &queue);
    VulkanSetObjectName(device, VK_OBJECT_TYPE_QUEUE, (uint64_t)queue, "Main Queue", vkSetDebugUtilsObjectNameEXT);

    // Swapchain
    VkSwapchainCreateInfoKHR swapchain_info;
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.pNext = NULL;
    swapchain_info.flags = 0;
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = (3 > surface_caps.maxImageCount ? surface_caps.maxImageCount : 3) < surface_caps.minImageCount ? surface_caps.minImageCount : 3;
    swapchain_info.imageFormat = surface_format;
    swapchain_info.imageColorSpace = surface_color_space;
    swapchain_info.imageExtent = surface_caps.currentExtent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = surface_usage;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.queueFamilyIndexCount = 1;
    swapchain_info.pQueueFamilyIndices = &queue_index;
    swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = surface_present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateSwapchainKHR(device, &swapchain_info, NULL, &swapchain));

    // Swapchain images
    uint32_t swapchain_image_count = 0;
    VK_CHECK_RES(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, NULL));
    assert(swapchain_image_count >= VULKAN_MAX_FRAMES_IN_FLIGHT);
    VkImage* swapchain_images = (VkImage*)malloc(swapchain_image_count * sizeof(VkImage));
    VK_CHECK_RES(vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images));
    for (uint32_t i = 0; i < swapchain_image_count; i++)
    {
        memset(object_name, 0, 128);
        sprintf(object_name, "Swapchain Image ");
        sprintf(object_name + 16, "%u", i);
        VulkanSetObjectName(device, VK_OBJECT_TYPE_IMAGE, (uint64_t)swapchain_images[i], object_name, vkSetDebugUtilsObjectNameEXT);
    }

    // Swapchain image views
    VkImageView* swapchain_image_views = (VkImageView*)malloc(swapchain_image_count * sizeof(VkImageView));
    for (uint32_t i = 0; i < swapchain_image_count; i++)
    {
        VkImageViewCreateInfo swapchain_image_view_info;
        swapchain_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        swapchain_image_view_info.pNext = NULL;
        swapchain_image_view_info.flags = 0;
        swapchain_image_view_info.image = swapchain_images[i];
        swapchain_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        swapchain_image_view_info.format = surface_format;
        swapchain_image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        swapchain_image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        swapchain_image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        swapchain_image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        swapchain_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        swapchain_image_view_info.subresourceRange.baseMipLevel = 0;
        swapchain_image_view_info.subresourceRange.levelCount = 1;
        swapchain_image_view_info.subresourceRange.baseArrayLayer = 0;
        swapchain_image_view_info.subresourceRange.layerCount = 1;
        VK_CHECK_RES(vkCreateImageView(device, &swapchain_image_view_info, NULL, &swapchain_image_views[i]));
        
        memset(object_name, 0, 128);
        sprintf(object_name, "Swapchain Image View ");
        sprintf(object_name + 21, "%u", i);
        VulkanSetObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)swapchain_image_views[i], object_name, vkSetDebugUtilsObjectNameEXT);
    }

    // Depth/Stencil image
    VkFormat depth_stencil_format = VK_FORMAT_D24_UNORM_S8_UINT;
    // Create image
    VkImageCreateInfo depth_stencil_image_info;
    depth_stencil_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_stencil_image_info.pNext = NULL;
    depth_stencil_image_info.flags = 0;
    depth_stencil_image_info.imageType = VK_IMAGE_TYPE_2D;
    depth_stencil_image_info.format = depth_stencil_format;
    depth_stencil_image_info.extent = { surface_caps.currentExtent.width, surface_caps.currentExtent.height, 1 };
    depth_stencil_image_info.mipLevels = 1;
    depth_stencil_image_info.arrayLayers = 1;
    depth_stencil_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_stencil_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_stencil_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth_stencil_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depth_stencil_image_info.queueFamilyIndexCount = 0;
    depth_stencil_image_info.pQueueFamilyIndices = NULL;
    depth_stencil_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImage depth_stencil_image = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateImage(device, &depth_stencil_image_info, NULL, &depth_stencil_image));
    // Allocate memory
    VkMemoryRequirements depth_stencil_image_memory_req;
    vkGetImageMemoryRequirements(device, depth_stencil_image, &depth_stencil_image_memory_req);
    uint32_t depth_stencil_image_memory_type = (uint32_t)-1;
    for (uint32_t i = 0; i < physical_device_memory_props.memoryTypeCount; i++)
    {
        // Buffer's memory supports this memory type
        if ((depth_stencil_image_memory_req.memoryTypeBits >> i) & 1)
        {
            if (physical_device_memory_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            {
                // TODO (Daniel): we simply choose the first type that fits our purpose...should be smarter about it?
                depth_stencil_image_memory_type = i;
                break;
            }
        }
    }
    assert(depth_stencil_image_memory_type != (uint32_t)-1);
    VkMemoryAllocateInfo depth_stencil_image_memory_info;
    depth_stencil_image_memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    depth_stencil_image_memory_info.pNext = NULL;
    depth_stencil_image_memory_info.allocationSize = depth_stencil_image_memory_req.size;
    depth_stencil_image_memory_info.memoryTypeIndex = depth_stencil_image_memory_type;
    VkDeviceMemory depth_stencil_image_memory = VK_NULL_HANDLE;
    VK_CHECK_RES(vkAllocateMemory(device, &depth_stencil_image_memory_info, NULL, &depth_stencil_image_memory));
    // Bind memory to image
    VK_CHECK_RES(vkBindImageMemory(device, depth_stencil_image, depth_stencil_image_memory, 0));
    // Create image view
    VkImageViewCreateInfo depth_stencil_image_view_info;
    depth_stencil_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_stencil_image_view_info.pNext = NULL;
    depth_stencil_image_view_info.flags = 0;
    depth_stencil_image_view_info.image = depth_stencil_image;
    depth_stencil_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_stencil_image_view_info.format = depth_stencil_format;
    depth_stencil_image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth_stencil_image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth_stencil_image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth_stencil_image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth_stencil_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    depth_stencil_image_view_info.subresourceRange.baseMipLevel = 0;
    depth_stencil_image_view_info.subresourceRange.levelCount = 1;
    depth_stencil_image_view_info.subresourceRange.baseArrayLayer = 0;
    depth_stencil_image_view_info.subresourceRange.layerCount = 1;
    VkImageView depth_stencil_image_view = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateImageView(device, &depth_stencil_image_view_info, NULL, &depth_stencil_image_view));
    VulkanSetObjectName(device, VK_OBJECT_TYPE_IMAGE, (uint64_t)depth_stencil_image, "Depth/Stencil Image", vkSetDebugUtilsObjectNameEXT);
    VulkanSetObjectName(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)depth_stencil_image_memory, "Depth/Stencil Image Memory", vkSetDebugUtilsObjectNameEXT);
    VulkanSetObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)depth_stencil_image_view, "Depth/Stencil Image View", vkSetDebugUtilsObjectNameEXT);

    // Intermediate swapchain image
    VkFormat intermediate_swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;
    // Create image
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = NULL;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = intermediate_swapchain_image_format;
    image_info.extent = { surface_caps.currentExtent.width, surface_caps.currentExtent.height, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = NULL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImage intermediate_swapchain_image = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateImage(device, &image_info, NULL, &intermediate_swapchain_image));
    // Allocate memory
    VkMemoryRequirements image_memory_req;
    vkGetImageMemoryRequirements(device, intermediate_swapchain_image, &image_memory_req);
    uint32_t image_memory_type = (uint32_t)-1;
    for (uint32_t i = 0; i < physical_device_memory_props.memoryTypeCount; i++)
    {
        // Buffer's memory supports this memory type
        if ((image_memory_req.memoryTypeBits >> i) & 1)
        {
            if (physical_device_memory_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            {
                // TODO (Daniel): we simply choose the first type that fits our purpose...should be smarter about it?
                image_memory_type = i;
                break;
            }
        }
    }
    assert(image_memory_type != (uint32_t)-1);
    VkMemoryAllocateInfo image_memory_info;
    image_memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    image_memory_info.pNext = NULL;
    image_memory_info.allocationSize = image_memory_req.size;
    image_memory_info.memoryTypeIndex = image_memory_type;
    VkDeviceMemory intermediate_swapchain_image_memory = VK_NULL_HANDLE;
    VK_CHECK_RES(vkAllocateMemory(device, &image_memory_info, NULL, &intermediate_swapchain_image_memory));
    // Bind memory to image
    VK_CHECK_RES(vkBindImageMemory(device, intermediate_swapchain_image, intermediate_swapchain_image_memory, 0));
    // Create image view
    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = NULL;
    image_view_info.flags = 0;
    image_view_info.image = intermediate_swapchain_image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = intermediate_swapchain_image_format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    VkImageView intermediate_swapchain_image_view = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateImageView(device, &image_view_info, NULL, &intermediate_swapchain_image_view));
    VulkanSetObjectName(device, VK_OBJECT_TYPE_IMAGE, (uint64_t)intermediate_swapchain_image, "Intermediate Swapchain Image", vkSetDebugUtilsObjectNameEXT);
    VulkanSetObjectName(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)intermediate_swapchain_image_memory, "Intermediate Swapchain Image Memory", vkSetDebugUtilsObjectNameEXT);
    VulkanSetObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)intermediate_swapchain_image_view, "Intermediate Swapchain Image View", vkSetDebugUtilsObjectNameEXT);

    // Command pool
    VkCommandPoolCreateInfo command_pool_info;
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.pNext = NULL;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = queue_index;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateCommandPool(device, &command_pool_info, NULL, &command_pool));

    // Command buffers - one per max_frames_in_flight
    VkCommandBufferAllocateInfo command_buffers_info;
    command_buffers_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffers_info.pNext = NULL;
    command_buffers_info.commandPool = command_pool;
    command_buffers_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffers_info.commandBufferCount = VULKAN_MAX_FRAMES_IN_FLIGHT;
    VkCommandBuffer* command_buffers = (VkCommandBuffer*)malloc(VULKAN_MAX_FRAMES_IN_FLIGHT * sizeof(VkCommandBuffer));
    VK_CHECK_RES(vkAllocateCommandBuffers(device, &command_buffers_info, command_buffers));
    for (uint32_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        memset(object_name, 0, 128);
        sprintf(object_name, "Command Buffer ");
        sprintf(object_name + 15, "%u", i);
        VulkanSetObjectName(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)command_buffers[i], object_name, vkSetDebugUtilsObjectNameEXT);
    }

    // Synchronization objects
    VkSemaphore* semaphores_image_available = (VkSemaphore*)malloc(VULKAN_MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    VkSemaphore* semaphores_render_finished = (VkSemaphore*)malloc(VULKAN_MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    VkFence* fences_frame_in_flight = (VkFence*)malloc(VULKAN_MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));
    for (uint32_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
    {
        // semaphores_image_available
        VkSemaphoreCreateInfo semaphore_info;
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_info.pNext = NULL;
        semaphore_info.flags = 0;
        VK_CHECK_RES(vkCreateSemaphore(device, &semaphore_info, NULL, &semaphores_image_available[i]));

        // semaphores_render_finished
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_info.pNext = NULL;
        semaphore_info.flags = 0;
        VK_CHECK_RES(vkCreateSemaphore(device, &semaphore_info, NULL, &semaphores_render_finished[i]));

        // fences_render_finished
        VkFenceCreateInfo fence_info;
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.pNext = NULL;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK_RES(vkCreateFence(device, &fence_info, NULL, &fences_frame_in_flight[i]));
        
        memset(object_name, 0, 128);
        sprintf(object_name, "Image Available Semaphore ");
        sprintf(object_name + 26, "%u", i);
        VulkanSetObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphores_image_available[i], object_name, vkSetDebugUtilsObjectNameEXT);
        memset(object_name, 0, 128);
        sprintf(object_name, "Render Finished Semaphore ");
        sprintf(object_name + 26, "%u", i);
        VulkanSetObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphores_render_finished[i], object_name, vkSetDebugUtilsObjectNameEXT);
        memset(object_name, 0, 128);
        sprintf(object_name, "Frame in Flight Fence ");
        sprintf(object_name + 22, "%u", i);
        VulkanSetObjectName(device, VK_OBJECT_TYPE_FENCE, (uint64_t)fences_frame_in_flight[i], object_name, vkSetDebugUtilsObjectNameEXT);
    }

    // Pipeline cache header
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_props);
    VkPipelineCacheHeaderVersionOne pipeline_cache_header;
    pipeline_cache_header.headerSize = sizeof(VkPipelineCacheHeaderVersionOne);
    pipeline_cache_header.headerVersion = VK_PIPELINE_CACHE_HEADER_VERSION_ONE;
    pipeline_cache_header.vendorID = physical_device_props.vendorID;
    pipeline_cache_header.deviceID = physical_device_props.deviceID;
    memcpy(pipeline_cache_header.pipelineCacheUUID, physical_device_props.pipelineCacheUUID, VK_UUID_SIZE);

    // Pipeline cache
    // Check pipeline cache data exists on disk for given GPU+Driver combination
    size_t pipeline_cache_file_size = 0;
    byte* pipeline_cache_file_data = NULL;
    byte* pipeline_cache_data = NULL;
    FILE* pipeline_cache_file = fopen("data/pipeline_cache.bin", "rb");
    if (pipeline_cache_file != NULL)
    {
        // Read in pipeline cache data (includes header)
        fseek(pipeline_cache_file, 0, SEEK_END);
        pipeline_cache_file_size = ftell(pipeline_cache_file);
        fseek(pipeline_cache_file, 0, SEEK_SET);
        pipeline_cache_file_data = (byte*)malloc(pipeline_cache_file_size);
        fread(pipeline_cache_file_data, pipeline_cache_file_size, 1, pipeline_cache_file);
        fclose(pipeline_cache_file);

        // Verify header matches current header
        VkPipelineCacheHeaderVersionOne* pipeline_cache_file_header = (VkPipelineCacheHeaderVersionOne*)pipeline_cache_file_data;
        assert(pipeline_cache_file_header->headerVersion == VK_PIPELINE_CACHE_HEADER_VERSION_ONE);
        if ((pipeline_cache_file_header->vendorID == pipeline_cache_header.vendorID) &&
            (pipeline_cache_file_header->deviceID == pipeline_cache_header.deviceID) &&
            (memcmp(pipeline_cache_file_header->pipelineCacheUUID, pipeline_cache_header.pipelineCacheUUID, VK_UUID_SIZE) == 0))
        {
            // Skip header if match
            pipeline_cache_data = pipeline_cache_file_data + sizeof(VkPipelineCacheHeaderVersionOne);
            pipeline_cache_file_size = pipeline_cache_file_size - sizeof(VkPipelineCacheHeaderVersionOne);
        }
        else
        {
            pipeline_cache_file_size = 0;
        }
    }
    // Create
    VkPipelineCacheCreateInfo pipeline_cache_info;
    pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipeline_cache_info.pNext = NULL;
    pipeline_cache_info.flags = 0;
    pipeline_cache_info.initialDataSize = pipeline_cache_file_size;
    pipeline_cache_info.pInitialData = (void*)pipeline_cache_data;
    VkPipelineCache pipeline_cache = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreatePipelineCache(device, &pipeline_cache_info, NULL, &pipeline_cache));
    VulkanSetObjectName(device, VK_OBJECT_TYPE_PIPELINE_CACHE, (uint64_t)pipeline_cache, "Pipeline Cache", vkSetDebugUtilsObjectNameEXT);
    if (pipeline_cache_file_data != NULL)
    {
        free(pipeline_cache_file_data);
    }

    // Transition swapchain images from LAYOUT_UNDEFINED to LAYOUT_PRESENT_SRC_KHR
    // Transition depth/stencil image from LAYOUT_UNDEFINED to LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    // Transition intermediate swapchain image from LAYOUT_UNDEFINED to LAYOUT_TRANSFER_SRC_OPTIMAL
    {
        VkCommandBuffer command_buffer = command_buffers[0];
        VkCommandBufferBeginInfo command_buffer_begin_info;
        command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.pNext = NULL;
        command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        command_buffer_begin_info.pInheritanceInfo = NULL;
        vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        VkImageMemoryBarrier* image_barriers = (VkImageMemoryBarrier*)malloc((swapchain_image_count + 2) * sizeof(VkImageMemoryBarrier));
        for (uint32_t i = 0; i < swapchain_image_count; i++)
        {
            image_barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            image_barriers[i].pNext = NULL;
            image_barriers[i].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
            image_barriers[i].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            image_barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_barriers[i].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            image_barriers[i].srcQueueFamilyIndex = queue_index;
            image_barriers[i].dstQueueFamilyIndex = queue_index;
            image_barriers[i].image = swapchain_images[i];
            image_barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_barriers[i].subresourceRange.baseMipLevel = 0;
            image_barriers[i].subresourceRange.levelCount = 1;
            image_barriers[i].subresourceRange.baseArrayLayer = 0;
            image_barriers[i].subresourceRange.layerCount = 1;
        }
        image_barriers[swapchain_image_count + 0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_barriers[swapchain_image_count + 0].pNext = NULL;
        image_barriers[swapchain_image_count + 0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        image_barriers[swapchain_image_count + 0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        image_barriers[swapchain_image_count + 0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_barriers[swapchain_image_count + 0].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        image_barriers[swapchain_image_count + 0].srcQueueFamilyIndex = queue_index;
        image_barriers[swapchain_image_count + 0].dstQueueFamilyIndex = queue_index;
        image_barriers[swapchain_image_count + 0].image = depth_stencil_image;
        image_barriers[swapchain_image_count + 0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        image_barriers[swapchain_image_count + 0].subresourceRange.baseMipLevel = 0;
        image_barriers[swapchain_image_count + 0].subresourceRange.levelCount = 1;
        image_barriers[swapchain_image_count + 0].subresourceRange.baseArrayLayer = 0;
        image_barriers[swapchain_image_count + 0].subresourceRange.layerCount = 1;
        image_barriers[swapchain_image_count + 1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_barriers[swapchain_image_count + 1].pNext = NULL;
        image_barriers[swapchain_image_count + 1].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        image_barriers[swapchain_image_count + 1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        image_barriers[swapchain_image_count + 1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_barriers[swapchain_image_count + 1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        image_barriers[swapchain_image_count + 1].srcQueueFamilyIndex = queue_index;
        image_barriers[swapchain_image_count + 1].dstQueueFamilyIndex = queue_index;
        image_barriers[swapchain_image_count + 1].image = intermediate_swapchain_image;
        image_barriers[swapchain_image_count + 1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_barriers[swapchain_image_count + 1].subresourceRange.baseMipLevel = 0;
        image_barriers[swapchain_image_count + 1].subresourceRange.levelCount = 1;
        image_barriers[swapchain_image_count + 1].subresourceRange.baseArrayLayer = 0;
        image_barriers[swapchain_image_count + 1].subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, swapchain_image_count + 2, image_barriers);
        vkEndCommandBuffer(command_buffer);
        VkSubmitInfo submit_info;
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = NULL;
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = NULL;
        submit_info.pWaitDstStageMask = NULL;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = NULL;
        VK_CHECK_RES(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));
        VK_CHECK_RES(vkQueueWaitIdle(queue));
        free(image_barriers);
    }

    // Update vulkan_context
    vulkan->target_api_version                 = target_api_version;
    vulkan->instance_api_version                = instance_api_version;
    vulkan->physical_device_api_version         = physical_device_props.apiVersion;
    vulkan->physical_device                     = physical_device;
    vulkan->physical_device_props               = physical_device_props;
    vulkan->physical_device_memory_props        = physical_device_memory_props;
    vulkan->physical_device_format_props        = physical_device_format_props;
    vulkan->surface                             = surface;
    vulkan->surface_caps                        = surface_caps;
    vulkan->surface_format                      = surface_format;
    vulkan->surface_color_space                 = surface_color_space;
    vulkan->surface_present_mode                = surface_present_mode;
    vulkan->queue_index                         = queue_index;
    vulkan->queue                               = queue;
    vulkan->device                              = device;
    vulkan->swapchain                           = swapchain;
    vulkan->swapchain_image_count               = swapchain_image_count;
    vulkan->swapchain_images                    = swapchain_images;
    vulkan->swapchain_image_views               = swapchain_image_views;
    vulkan->depth_stencil_format                = depth_stencil_format;
    vulkan->depth_stencil_image                 = depth_stencil_image;
    vulkan->depth_stencil_image_memory          = depth_stencil_image_memory;
    vulkan->depth_stencil_image_view            = depth_stencil_image_view;
    vulkan->intermediate_swapchain_image_format = intermediate_swapchain_image_format;
    vulkan->intermediate_swapchain_image        = intermediate_swapchain_image;
    vulkan->intermediate_swapchain_image_memory = intermediate_swapchain_image_memory;
    vulkan->intermediate_swapchain_image_view   = intermediate_swapchain_image_view;
    vulkan->command_buffers                     = command_buffers;
    vulkan->semaphores_image_available          = semaphores_image_available;
    vulkan->semaphores_render_finished          = semaphores_render_finished;
    vulkan->fences_frame_in_flight              = fences_frame_in_flight;
    vulkan->pipeline_cache_header               = pipeline_cache_header;
    vulkan->pipeline_cache                      = pipeline_cache;
    vulkan->vkSetDebugUtilsObjectNameEXT        = vkSetDebugUtilsObjectNameEXT;
    vulkan->vkCmdBeginDebugUtilsLabelEXT        = vkCmdBeginDebugUtilsLabelEXT;
    vulkan->vkCmdEndDebugUtilsLabelEXT          = vkCmdEndDebugUtilsLabelEXT;
}

void VulkanSetObjectName(VkDevice device, VkObjectType object_type, uint64_t object, const char* name, PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT)
{
    VkDebugUtilsObjectNameInfoEXT name_info;
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.pNext = NULL;
    name_info.objectType = object_type;
    name_info.objectHandle = object;
    name_info.pObjectName = name;
    VK_CHECK_RES(vkSetDebugUtilsObjectNameEXT(device, &name_info));
}

void VulkanSetObjectName(vulkan_context_t* vulkan, VkObjectType object_type, uint64_t object, const char* name)
{
    VulkanSetObjectName(vulkan->device, object_type, object, name, vulkan->vkSetDebugUtilsObjectNameEXT);
}

void VulkanCleanUpSwapchain(vulkan_context_t* vulkan)
{
    // Intermeditae swapchain image
    vkDestroyImageView(vulkan->device, vulkan->intermediate_swapchain_image_view, NULL);
    vkFreeMemory(vulkan->device, vulkan->intermediate_swapchain_image_memory, NULL);
    vkDestroyImage(vulkan->device, vulkan->intermediate_swapchain_image, NULL);

    // Depth-Stencil image
    vkDestroyImageView(vulkan->device, vulkan->depth_stencil_image_view, NULL);
    vkFreeMemory(vulkan->device, vulkan->depth_stencil_image_memory, NULL);
    vkDestroyImage(vulkan->device, vulkan->depth_stencil_image, NULL);

    // Swapchain image views
    for (uint32_t i = 0; i < vulkan->swapchain_image_count; i++)
    {
        vkDestroyImageView(vulkan->device, vulkan->swapchain_image_views[i], NULL);
    }
    free(vulkan->swapchain_images);

    // Swapchain
    vkDestroySwapchainKHR(vulkan->device, vulkan->swapchain, NULL);
}

void VulkanRecreateSwapchain(vulkan_context_t* vulkan)
{
    char object_name[128];

    // Surface capabilities
    const VkImageUsageFlags surface_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    VkSurfaceCapabilitiesKHR surface_caps;
    VK_CHECK_RES(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan->physical_device, vulkan->surface, &surface_caps));
    assert(surface_caps.supportedUsageFlags & surface_usage);
    printf("\tSurface capabilities:\n"
           "\t\tMin image count: %u\n"
           "\t\tMax image count: %u\n"
           "\t\tMinimum extent: %ux%u\n"
           "\t\tMaximum extent: %ux%u\n"
           "\t\tCurrent extent: %ux%u\n",
           surface_caps.minImageCount,
           surface_caps.maxImageCount,
           surface_caps.minImageExtent.width, surface_caps.minImageExtent.height,
           surface_caps.maxImageExtent.width, surface_caps.maxImageExtent.height,
           surface_caps.currentExtent.width, surface_caps.currentExtent.height);

    // Surface format and color space
    // Since nothing except the size of the surface has changed, we know the same format and present mode
    // will work on the current surface
    const VkFormat surface_format = VK_FORMAT_B8G8R8A8_SRGB;
    const VkColorSpaceKHR surface_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    const VkPresentModeKHR surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    // Swapchain
    VkSwapchainCreateInfoKHR swapchain_info;
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.pNext = NULL;
    swapchain_info.flags = 0;
    swapchain_info.surface = vulkan->surface;
    swapchain_info.minImageCount = (3 > surface_caps.maxImageCount ? surface_caps.maxImageCount : 3) < surface_caps.minImageCount ? surface_caps.minImageCount : 3;
    swapchain_info.imageFormat = surface_format;
    swapchain_info.imageColorSpace = surface_color_space;
    swapchain_info.imageExtent = surface_caps.currentExtent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = surface_usage;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.queueFamilyIndexCount = 1;
    swapchain_info.pQueueFamilyIndices = &vulkan->queue_index;
    swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = surface_present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateSwapchainKHR(vulkan->device, &swapchain_info, NULL, &swapchain));

    // Swapchain images
    uint32_t swapchain_image_count = 0;
    VK_CHECK_RES(vkGetSwapchainImagesKHR(vulkan->device, swapchain, &swapchain_image_count, NULL));
    assert(swapchain_image_count >= VULKAN_MAX_FRAMES_IN_FLIGHT);
    VkImage* swapchain_images = (VkImage*)malloc(swapchain_image_count * sizeof(VkImage));
    VK_CHECK_RES(vkGetSwapchainImagesKHR(vulkan->device, swapchain, &swapchain_image_count, swapchain_images));
    for (uint32_t i = 0; i < swapchain_image_count; i++)
    {
        memset(object_name, 0, 128);
        sprintf(object_name, "Swapchain Image ");
        sprintf(object_name + 16, "%u", i);
        VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)swapchain_images[i], object_name, vulkan->vkSetDebugUtilsObjectNameEXT);
    }

    // Swapchain image views
    VkImageView* swapchain_image_views = (VkImageView*)malloc(swapchain_image_count * sizeof(VkImageView));
    for (uint32_t i = 0; i < swapchain_image_count; i++)
    {
        VkImageViewCreateInfo swapchain_image_view_info;
        swapchain_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        swapchain_image_view_info.pNext = NULL;
        swapchain_image_view_info.flags = 0;
        swapchain_image_view_info.image = swapchain_images[i];
        swapchain_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        swapchain_image_view_info.format = surface_format;
        swapchain_image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        swapchain_image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        swapchain_image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        swapchain_image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        swapchain_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        swapchain_image_view_info.subresourceRange.baseMipLevel = 0;
        swapchain_image_view_info.subresourceRange.levelCount = 1;
        swapchain_image_view_info.subresourceRange.baseArrayLayer = 0;
        swapchain_image_view_info.subresourceRange.layerCount = 1;
        VK_CHECK_RES(vkCreateImageView(vulkan->device, &swapchain_image_view_info, NULL, &swapchain_image_views[i]));
        
        memset(object_name, 0, 128);
        sprintf(object_name, "Swapchain Image View ");
        sprintf(object_name + 21, "%u", i);
        VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)swapchain_image_views[i], object_name, vulkan->vkSetDebugUtilsObjectNameEXT);
    }

    // Depth/Stencil image
    VkFormat depth_stencil_format = VK_FORMAT_D24_UNORM_S8_UINT;
    // Create image
    VkImageCreateInfo depth_stencil_image_info;
    depth_stencil_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_stencil_image_info.pNext = NULL;
    depth_stencil_image_info.flags = 0;
    depth_stencil_image_info.imageType = VK_IMAGE_TYPE_2D;
    depth_stencil_image_info.format = depth_stencil_format;
    depth_stencil_image_info.extent = { surface_caps.currentExtent.width, surface_caps.currentExtent.height, 1 };
    depth_stencil_image_info.mipLevels = 1;
    depth_stencil_image_info.arrayLayers = 1;
    depth_stencil_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_stencil_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_stencil_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth_stencil_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depth_stencil_image_info.queueFamilyIndexCount = 0;
    depth_stencil_image_info.pQueueFamilyIndices = NULL;
    depth_stencil_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImage depth_stencil_image = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateImage(vulkan->device, &depth_stencil_image_info, NULL, &depth_stencil_image));
    // Allocate memory
    VkMemoryRequirements depth_stencil_image_memory_req;
    vkGetImageMemoryRequirements(vulkan->device, depth_stencil_image, &depth_stencil_image_memory_req);
    uint32_t depth_stencil_image_memory_type = (uint32_t)-1;
    for (uint32_t i = 0; i < vulkan->physical_device_memory_props.memoryTypeCount; i++)
    {
        // Buffer's memory supports this memory type
        if ((depth_stencil_image_memory_req.memoryTypeBits >> i) & 1)
        {
            if (vulkan->physical_device_memory_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            {
                // TODO (Daniel): we simply choose the first type that fits our purpose...should be smarter about it?
                depth_stencil_image_memory_type = i;
                break;
            }
        }
    }
    assert(depth_stencil_image_memory_type != (uint32_t)-1);
    VkMemoryAllocateInfo depth_stencil_image_memory_info;
    depth_stencil_image_memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    depth_stencil_image_memory_info.pNext = NULL;
    depth_stencil_image_memory_info.allocationSize = depth_stencil_image_memory_req.size;
    depth_stencil_image_memory_info.memoryTypeIndex = depth_stencil_image_memory_type;
    VkDeviceMemory depth_stencil_image_memory = VK_NULL_HANDLE;
    VK_CHECK_RES(vkAllocateMemory(vulkan->device, &depth_stencil_image_memory_info, NULL, &depth_stencil_image_memory));
    // Bind memory to image
    VK_CHECK_RES(vkBindImageMemory(vulkan->device, depth_stencil_image, depth_stencil_image_memory, 0));
    // Create image view
    VkImageViewCreateInfo depth_stencil_image_view_info;
    depth_stencil_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_stencil_image_view_info.pNext = NULL;
    depth_stencil_image_view_info.flags = 0;
    depth_stencil_image_view_info.image = depth_stencil_image;
    depth_stencil_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_stencil_image_view_info.format = depth_stencil_format;
    depth_stencil_image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth_stencil_image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth_stencil_image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth_stencil_image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    depth_stencil_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    depth_stencil_image_view_info.subresourceRange.baseMipLevel = 0;
    depth_stencil_image_view_info.subresourceRange.levelCount = 1;
    depth_stencil_image_view_info.subresourceRange.baseArrayLayer = 0;
    depth_stencil_image_view_info.subresourceRange.layerCount = 1;
    VkImageView depth_stencil_image_view = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateImageView(vulkan->device, &depth_stencil_image_view_info, NULL, &depth_stencil_image_view));
    VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)depth_stencil_image, "Depth/Stencil Image", vulkan->vkSetDebugUtilsObjectNameEXT);
    VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)depth_stencil_image_memory, "Depth/Stencil Image Memory", vulkan->vkSetDebugUtilsObjectNameEXT);
    VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)depth_stencil_image_view, "Depth/Stencil Image View", vulkan->vkSetDebugUtilsObjectNameEXT);

    // Intermediate swapchain image
    VkFormat intermediate_swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;
    // Create image
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = NULL;
    image_info.flags = 0;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = intermediate_swapchain_image_format;
    image_info.extent = { surface_caps.currentExtent.width, surface_caps.currentExtent.height, 1 };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = NULL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImage intermediate_swapchain_image = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateImage(vulkan->device, &image_info, NULL, &intermediate_swapchain_image));
    // Allocate memory
    VkMemoryRequirements image_memory_req;
    vkGetImageMemoryRequirements(vulkan->device, intermediate_swapchain_image, &image_memory_req);
    uint32_t image_memory_type = (uint32_t)-1;
    for (uint32_t i = 0; i < vulkan->physical_device_memory_props.memoryTypeCount; i++)
    {
        // Buffer's memory supports this memory type
        if ((image_memory_req.memoryTypeBits >> i) & 1)
        {
            if (vulkan->physical_device_memory_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            {
                // TODO (Daniel): we simply choose the first type that fits our purpose...should be smarter about it?
                image_memory_type = i;
                break;
            }
        }
    }
    assert(image_memory_type != (uint32_t)-1);
    VkMemoryAllocateInfo image_memory_info;
    image_memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    image_memory_info.pNext = NULL;
    image_memory_info.allocationSize = image_memory_req.size;
    image_memory_info.memoryTypeIndex = image_memory_type;
    VkDeviceMemory intermediate_swapchain_image_memory = VK_NULL_HANDLE;
    VK_CHECK_RES(vkAllocateMemory(vulkan->device, &image_memory_info, NULL, &intermediate_swapchain_image_memory));
    // Bind memory to image
    VK_CHECK_RES(vkBindImageMemory(vulkan->device, intermediate_swapchain_image, intermediate_swapchain_image_memory, 0));
    // Create image view
    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = NULL;
    image_view_info.flags = 0;
    image_view_info.image = intermediate_swapchain_image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = intermediate_swapchain_image_format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    VkImageView intermediate_swapchain_image_view = VK_NULL_HANDLE;
    VK_CHECK_RES(vkCreateImageView(vulkan->device, &image_view_info, NULL, &intermediate_swapchain_image_view));
    VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)intermediate_swapchain_image, "Intermediate Swapchain Image", vulkan->vkSetDebugUtilsObjectNameEXT);
    VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)intermediate_swapchain_image_memory, "Intermediate Swapchain Image Memory", vulkan->vkSetDebugUtilsObjectNameEXT);
    VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)intermediate_swapchain_image_view, "Intermediate Swapchain Image View", vulkan->vkSetDebugUtilsObjectNameEXT);

    // Transfer images to correct initial layout
    VkCommandBuffer command_buffer = vulkan->command_buffers[0];
    VkCommandBufferBeginInfo command_buffer_begin_info;
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = NULL;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = NULL;
    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    VkImageMemoryBarrier* image_barriers = (VkImageMemoryBarrier*)malloc((swapchain_image_count + 2) * sizeof(VkImageMemoryBarrier));
    for (uint32_t i = 0; i < swapchain_image_count; i++)
    {
        image_barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_barriers[i].pNext = NULL;
        image_barriers[i].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        image_barriers[i].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        image_barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_barriers[i].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        image_barriers[i].srcQueueFamilyIndex = vulkan->queue_index;
        image_barriers[i].dstQueueFamilyIndex = vulkan->queue_index;
        image_barriers[i].image = swapchain_images[i];
        image_barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_barriers[i].subresourceRange.baseMipLevel = 0;
        image_barriers[i].subresourceRange.levelCount = 1;
        image_barriers[i].subresourceRange.baseArrayLayer = 0;
        image_barriers[i].subresourceRange.layerCount = 1;
    }
    image_barriers[swapchain_image_count + 0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barriers[swapchain_image_count + 0].pNext = NULL;
    image_barriers[swapchain_image_count + 0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    image_barriers[swapchain_image_count + 0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    image_barriers[swapchain_image_count + 0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_barriers[swapchain_image_count + 0].newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    image_barriers[swapchain_image_count + 0].srcQueueFamilyIndex = vulkan->queue_index;
    image_barriers[swapchain_image_count + 0].dstQueueFamilyIndex = vulkan->queue_index;
    image_barriers[swapchain_image_count + 0].image = depth_stencil_image;
    image_barriers[swapchain_image_count + 0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    image_barriers[swapchain_image_count + 0].subresourceRange.baseMipLevel = 0;
    image_barriers[swapchain_image_count + 0].subresourceRange.levelCount = 1;
    image_barriers[swapchain_image_count + 0].subresourceRange.baseArrayLayer = 0;
    image_barriers[swapchain_image_count + 0].subresourceRange.layerCount = 1;
    image_barriers[swapchain_image_count + 1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barriers[swapchain_image_count + 1].pNext = NULL;
    image_barriers[swapchain_image_count + 1].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    image_barriers[swapchain_image_count + 1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    image_barriers[swapchain_image_count + 1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_barriers[swapchain_image_count + 1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    image_barriers[swapchain_image_count + 1].srcQueueFamilyIndex = vulkan->queue_index;
    image_barriers[swapchain_image_count + 1].dstQueueFamilyIndex = vulkan->queue_index;
    image_barriers[swapchain_image_count + 1].image = intermediate_swapchain_image;
    image_barriers[swapchain_image_count + 1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barriers[swapchain_image_count + 1].subresourceRange.baseMipLevel = 0;
    image_barriers[swapchain_image_count + 1].subresourceRange.levelCount = 1;
    image_barriers[swapchain_image_count + 1].subresourceRange.baseArrayLayer = 0;
    image_barriers[swapchain_image_count + 1].subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, swapchain_image_count + 2, image_barriers);
    vkEndCommandBuffer(command_buffer);
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;
    VK_CHECK_RES(vkQueueSubmit(vulkan->queue, 1, &submit_info, VK_NULL_HANDLE));
    VK_CHECK_RES(vkQueueWaitIdle(vulkan->queue));
    free(image_barriers);

    // Update vulkan_context
    vulkan->swapchain = swapchain;
    vulkan->swapchain_image_count = swapchain_image_count;
    vulkan->swapchain_images = swapchain_images;
    vulkan->swapchain_image_views = swapchain_image_views;
    vulkan->depth_stencil_image = depth_stencil_image;
    vulkan->depth_stencil_image_memory = depth_stencil_image_memory;
    vulkan->depth_stencil_image_view = depth_stencil_image_view;
    vulkan->intermediate_swapchain_image = intermediate_swapchain_image;
    vulkan->intermediate_swapchain_image_memory = intermediate_swapchain_image_memory;
    vulkan->intermediate_swapchain_image_view = intermediate_swapchain_image_view;
}

void VulkanCreateShader(vulkan_context_t* vulkan, const char* file, VkShaderModule* shader, const char* shader_name)
{
    assert(vulkan != NULL);
    assert(file != NULL);
    assert(*shader == VK_NULL_HANDLE);

    // Load SPIR-V file
    FILE* spirv_file = fopen(file, "rb");
    assert(spirv_file != NULL);
    fseek(spirv_file, 0, SEEK_END);
    long spirv_size = ftell(spirv_file);
    fseek(spirv_file, 0, SEEK_SET);
    uint32_t* spirv = (uint32_t*)malloc(spirv_size);
    fread(spirv, 1, spirv_size, spirv_file);
    fclose(spirv_file);

    // Create shader module
    VkShaderModuleCreateInfo shader_info;
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.pNext = NULL;
    shader_info.flags = 0;
    shader_info.codeSize = spirv_size;
    shader_info.pCode = spirv;
    VK_CHECK_RES(vkCreateShaderModule(vulkan->device, &shader_info, NULL, shader));
    free(spirv);

    // Name
    if (shader_name != NULL)
    {
        VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)*shader, shader_name, vulkan->vkSetDebugUtilsObjectNameEXT);
    }
}

void VulkanCreateBuffer(vulkan_context_t* vulkan, void* buffer_data, VkDeviceSize buffer_data_size, VkBufferUsageFlags buffer_usage, VkMemoryPropertyFlags buffer_memory_properties, VkBuffer* buffer, VkDeviceMemory* buffer_memory, const char* buffer_name, const char* buffer_memory_name)
{
    assert(vulkan != NULL);
    assert(buffer_usage != 0);
    assert(*buffer == VK_NULL_HANDLE);
    assert(*buffer_memory == VK_NULL_HANDLE);

    // Create buffer
    VkBufferCreateInfo buffer_info;
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.pNext = NULL;
    buffer_info.flags = 0;
    buffer_info.size = buffer_data_size;
    buffer_info.usage = buffer_usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.queueFamilyIndexCount = 0;
    buffer_info.pQueueFamilyIndices = NULL;
    VK_CHECK_RES(vkCreateBuffer(vulkan->device, &buffer_info, NULL, buffer));

    // Allocate buffer memory
    VkMemoryRequirements buffer_memory_req;
    vkGetBufferMemoryRequirements(vulkan->device, *buffer, &buffer_memory_req);
    uint32_t buffer_memory_type = (uint32_t)-1;
    for (uint32_t i = 0; i < vulkan->physical_device_memory_props.memoryTypeCount; i++)
    {
        // Buffer's memory supports this memory type
        if ((buffer_memory_req.memoryTypeBits >> i) & 1)
        {
            if ((vulkan->physical_device_memory_props.memoryTypes[i].propertyFlags & buffer_memory_properties) == buffer_memory_properties)
            {
                // TODO (Daniel): we simply choose the first type that fits our purpose...should be smarter about it?
                buffer_memory_type = i;
                break;
            }
        }
    }
    if (buffer_memory_type == (uint32_t)-1)
    {
        printf("%s:%i Failed to find a matching buffer memory type\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    VkMemoryAllocateInfo buffer_memory_info;
    buffer_memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    buffer_memory_info.pNext = NULL;
    buffer_memory_info.allocationSize = buffer_memory_req.size;
    buffer_memory_info.memoryTypeIndex = buffer_memory_type;
    VK_CHECK_RES(vkAllocateMemory(vulkan->device, &buffer_memory_info, NULL, buffer_memory));

    // Bind buffer to memory
    VK_CHECK_RES(vkBindBufferMemory(vulkan->device, *buffer, *buffer_memory, 0));

    // Copy data to buffer if there is data to copy
    if (buffer_data != NULL)
    {
        // Stage the data to be copied if the buffer is not to be HOST_VISIBLE | HOST_COHERENT
        if ((buffer_memory_properties & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) != (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            // Create staging buffer
            buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            VkBuffer buffer_staging = VK_NULL_HANDLE;
            VK_CHECK_RES(vkCreateBuffer(vulkan->device, &buffer_info, NULL, &buffer_staging));

            // Allocate staging buffer memory
            vkGetBufferMemoryRequirements(vulkan->device, buffer_staging, &buffer_memory_req);
            buffer_memory_type = (uint32_t)-1;
            for (uint32_t i = 0; i < vulkan->physical_device_memory_props.memoryTypeCount; i++)
            {
                // Buffer's memory supports this memory type
                if ((buffer_memory_req.memoryTypeBits >> i) & 1)
                {
                    if ((vulkan->physical_device_memory_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                        (vulkan->physical_device_memory_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
                    {
                        // TODO (Daniel): we simply choose the first type that fits our purpose...should be smarter about it?
                        buffer_memory_type = i;
                        break;
                    }
                }
            }
            if (buffer_memory_type == (uint32_t)-1)
            {
                printf("%s:%i Failed to find a matching buffer memory type\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            buffer_memory_info.allocationSize = buffer_memory_req.size;
            buffer_memory_info.memoryTypeIndex = buffer_memory_type;
            VkDeviceMemory buffer_memory_staging = VK_NULL_HANDLE;
            VK_CHECK_RES(vkAllocateMemory(vulkan->device, &buffer_memory_info, NULL, &buffer_memory_staging));

            // Bind staging buffer to staging memory
            VK_CHECK_RES(vkBindBufferMemory(vulkan->device, buffer_staging, buffer_memory_staging, 0));

            // Copy data to staging buffer
            assert(buffer_data_size > 0);
            void* buffer_memory_ptr = NULL;
            VK_CHECK_RES(vkMapMemory(vulkan->device, buffer_memory_staging, 0, VK_WHOLE_SIZE, 0, &buffer_memory_ptr));
            assert(buffer_memory_ptr != NULL);
            memcpy(buffer_memory_ptr, buffer_data, buffer_data_size);
            vkUnmapMemory(vulkan->device, buffer_memory_staging);

            // Copy data from staging buffer to buffer
            VkCommandBuffer command_buffer = vulkan->command_buffers[0];
            VkCommandBufferBeginInfo command_buffer_begin_info;
            command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            command_buffer_begin_info.pNext = NULL;
            command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            command_buffer_begin_info.pInheritanceInfo = NULL;
            vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
            VkBufferCopy  buffer_copy_region;
            buffer_copy_region.srcOffset = 0;
            buffer_copy_region.dstOffset = 0;
            buffer_copy_region.size = (VkDeviceSize)buffer_data_size;
            vkCmdCopyBuffer(command_buffer, buffer_staging, *buffer, 1, &buffer_copy_region);
            vkEndCommandBuffer(command_buffer);
            VkSubmitInfo submit_info;
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pNext = NULL;
            submit_info.waitSemaphoreCount = 0;
            submit_info.pWaitSemaphores = NULL;
            submit_info.pWaitDstStageMask = NULL;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores = NULL;
            VK_CHECK_RES(vkQueueSubmit(vulkan->queue, 1, &submit_info, VK_NULL_HANDLE));
            VK_CHECK_RES(vkQueueWaitIdle(vulkan->queue));

            // Destroy staging resources
            vkFreeMemory(vulkan->device, buffer_memory_staging, NULL);
            vkDestroyBuffer(vulkan->device, buffer_staging, NULL);
        }
        else // Directly copy the data to the buffer
        {
            assert(buffer_data_size > 0);
            void* buffer_memory_ptr = NULL;
            VK_CHECK_RES(vkMapMemory(vulkan->device, *buffer_memory, 0, VK_WHOLE_SIZE, 0, &buffer_memory_ptr));
            assert(buffer_memory_ptr != NULL);
            memcpy(buffer_memory_ptr, buffer_data, buffer_data_size);
            vkUnmapMemory(vulkan->device, *buffer_memory);
        }
    }

    // Name
    if (buffer_name != NULL)
    {
        VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)*buffer, buffer_name, vulkan->vkSetDebugUtilsObjectNameEXT);
    }
    if (buffer_memory_name != NULL)
    {
        VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)*buffer_memory, buffer_memory_name, vulkan->vkSetDebugUtilsObjectNameEXT);
    }
}

void VulkanDestroyBuffer(vulkan_context_t* vulkan, VkBuffer* buffer, VkDeviceMemory* buffer_memory)
{
    vkFreeMemory(vulkan->device, *buffer_memory, NULL);
    vkDestroyBuffer(vulkan->device, *buffer, NULL);

    *buffer_memory = VK_NULL_HANDLE;
    *buffer = VK_NULL_HANDLE;
}

void VulkanCreateImage(vulkan_context_t* vulkan, VkImageType image_type, VkFormat image_format, uint32_t image_width, uint32_t image_height, uint32_t image_depth, VkSampleCountFlagBits image_samples, VkImageTiling image_tiling, VkImageUsageFlags image_usage, VkImageViewType image_view_type, VkImageAspectFlags image_aspect, VkImageLayout image_final_layout, void* image_data, VkDeviceSize image_data_size, VkMemoryPropertyFlags image_memory_properties, VkImage* image, VkDeviceMemory *image_memory, VkImageView* image_view, const char* image_name, const char* image_memory_name, const char* image_view_name)
{
        assert(vulkan != NULL);
    assert(*image == VK_NULL_HANDLE);
    assert(*image_memory == VK_NULL_HANDLE);
    assert(*image_view == VK_NULL_HANDLE);

    // Create image
    VkImageCreateInfo image_info;
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = NULL;
    image_info.flags = 0;
    image_info.imageType = image_type;
    image_info.format = image_format;
    image_info.extent = { image_width, image_height, image_depth };
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = image_samples;
    image_info.tiling = image_tiling;
    image_info.usage = image_usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = NULL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK_RES(vkCreateImage(vulkan->device, &image_info, NULL, image));
    VkImageLayout image_current_layout = VK_IMAGE_LAYOUT_UNDEFINED;

     // Allocate memory
    VkMemoryRequirements image_memory_req;
    vkGetImageMemoryRequirements(vulkan->device, *image, &image_memory_req);
    uint32_t image_memory_type = (uint32_t)-1;
    for (uint32_t i = 0; i < vulkan->physical_device_memory_props.memoryTypeCount; i++)
    {
        // Buffer's memory supports this memory type
        if ((image_memory_req.memoryTypeBits >> i) & 1)
        {
            // TODO (Daniel): Should be decided by an input parameter
            if ((vulkan->physical_device_memory_props.memoryTypes[i].propertyFlags & image_memory_properties) == image_memory_properties)
            {
                // TODO (Daniel): we simply choose the first type that fits our purpose...should be smarter about it?
                image_memory_type = i;
                break;
            }
        }
    }
    if (image_memory_type == (uint32_t)-1)
    {
        printf("%s:%i Failed to find a matching image memory type\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    VkMemoryAllocateInfo image_memory_info;
    image_memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    image_memory_info.pNext = NULL;
    image_memory_info.allocationSize = image_memory_req.size;
    image_memory_info.memoryTypeIndex = image_memory_type;
    VK_CHECK_RES(vkAllocateMemory(vulkan->device, &image_memory_info, NULL, image_memory));

    // Bind memory to image
    VK_CHECK_RES(vkBindImageMemory(vulkan->device, *image, *image_memory, 0));

    // Create image view
    VkImageViewCreateInfo image_view_info;
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = NULL;
    image_view_info.flags = 0;
    image_view_info.image = *image;
    image_view_info.viewType = image_view_type;
    image_view_info.format = image_format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = image_aspect;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    VK_CHECK_RES(vkCreateImageView(vulkan->device, &image_view_info, NULL, image_view));

    // Copy data to buffer if there is data to copy
    if (image_data != NULL)
    {
        // Stage the data to be copied if the buffer is not to be HOST_VISIBLE | HOST_COHERENT, or if image tiling isn't LINEAR
        if ((image_memory_properties & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) != (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
            (image_tiling != VK_IMAGE_TILING_LINEAR))
        {
            // Create buffer
            VkBufferCreateInfo buffer_info;
            buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buffer_info.pNext = NULL;
            buffer_info.flags = 0;
            buffer_info.size = image_data_size;
            buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            buffer_info.queueFamilyIndexCount = 0;
            buffer_info.pQueueFamilyIndices = NULL;
            VkBuffer buffer_staging = VK_NULL_HANDLE;
            VK_CHECK_RES(vkCreateBuffer(vulkan->device, &buffer_info, NULL, &buffer_staging));

            // Allocate buffer memory
            VkMemoryRequirements buffer_memory_req;
            vkGetBufferMemoryRequirements(vulkan->device, buffer_staging, &buffer_memory_req);
            uint32_t buffer_memory_type = (uint32_t)-1;
            for (uint32_t i = 0; i < vulkan->physical_device_memory_props.memoryTypeCount; i++)
            {
                // Buffer's memory supports this memory type
                if ((buffer_memory_req.memoryTypeBits >> i) & 1)
                {
                    if ((vulkan->physical_device_memory_props.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
                    {
                        // TODO (Daniel): we simply choose the first type that fits our purpose...should be smarter about it?
                        buffer_memory_type = i;
                        break;
                    }
                }
            }
            if (buffer_memory_type == (uint32_t)-1)
            {
                printf("%s:%i Failed to find a matching buffer memory type\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            VkMemoryAllocateInfo buffer_memory_info;
            buffer_memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            buffer_memory_info.pNext = NULL;
            buffer_memory_info.allocationSize = buffer_memory_req.size;
            buffer_memory_info.memoryTypeIndex = buffer_memory_type;
            VkDeviceMemory buffer_memory_staging = VK_NULL_HANDLE;
            VK_CHECK_RES(vkAllocateMemory(vulkan->device, &buffer_memory_info, NULL, &buffer_memory_staging));

            // Bind buffer to memory
            VK_CHECK_RES(vkBindBufferMemory(vulkan->device, buffer_staging, buffer_memory_staging, 0));

            // Copy image data to staging buffer
            void* buffer_memory_ptr = NULL;
            VK_CHECK_RES(vkMapMemory(vulkan->device, buffer_memory_staging, 0, VK_WHOLE_SIZE, 0, &buffer_memory_ptr));
            assert(buffer_memory_ptr != NULL);
            memcpy(buffer_memory_ptr, image_data, image_data_size);
            vkUnmapMemory(vulkan->device, buffer_memory_staging);

            // Transfer image from UNDEFINED -> TRANSFER_DST
            VkCommandBuffer command_buffer = vulkan->command_buffers[0];
            VkCommandBufferBeginInfo command_buffer_begin_info;
            command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            command_buffer_begin_info.pNext = NULL;
            command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            command_buffer_begin_info.pInheritanceInfo = NULL;
            vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
            VulkanCmdTransitionImageLayout(vulkan, command_buffer, *image, image_current_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, 0);
            image_current_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            // Copy staging image to image
            VkBufferImageCopy buffer_image_copy_region;
            buffer_image_copy_region.bufferOffset = 0;
            buffer_image_copy_region.bufferRowLength = 0;
            buffer_image_copy_region.bufferImageHeight = 0;
            buffer_image_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            buffer_image_copy_region.imageSubresource.mipLevel = 0;
            buffer_image_copy_region.imageSubresource.baseArrayLayer = 0;
            buffer_image_copy_region.imageSubresource.layerCount = 1;
            buffer_image_copy_region.imageOffset = { 0, 0, 0 };
            buffer_image_copy_region.imageExtent = { image_width, image_height, image_depth };
            vkCmdCopyBufferToImage(command_buffer, buffer_staging, *image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy_region);
            vkEndCommandBuffer(command_buffer);
            VkSubmitInfo submit_info;
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.pNext = NULL;
            submit_info.waitSemaphoreCount = 0;
            submit_info.pWaitSemaphores = NULL;
            submit_info.pWaitDstStageMask = NULL;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffer;
            submit_info.signalSemaphoreCount = 0;
            submit_info.pSignalSemaphores = NULL;
            VK_CHECK_RES(vkQueueSubmit(vulkan->queue, 1, &submit_info, VK_NULL_HANDLE));
            VK_CHECK_RES(vkQueueWaitIdle(vulkan->queue));

            // Destroy staging resources
            vkFreeMemory(vulkan->device, buffer_memory_staging, NULL);
            vkDestroyBuffer(vulkan->device, buffer_staging, NULL);
        }
        else // Copy the data directly
        {
            void* image_memory_ptr = NULL;
            VK_CHECK_RES(vkMapMemory(vulkan->device, *image_memory, 0, VK_WHOLE_SIZE, 0, &image_memory_ptr));
            assert(image_memory_ptr != NULL);
            memcpy(image_memory_ptr, image_data, image_data_size);
            vkUnmapMemory(vulkan->device, *image_memory);
        }
    }

    // TODO (Daniel): extract
    // Transition image from image_current_layout to image_final_layout
    VkCommandBuffer command_buffer = vulkan->command_buffers[0];
    VkCommandBufferBeginInfo command_buffer_begin_info;
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = NULL;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = NULL;
    vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
    VulkanCmdTransitionImageLayout(vulkan, command_buffer, *image, image_current_layout, image_final_layout, image_aspect, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, 0);
    vkEndCommandBuffer(command_buffer);
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;
    VK_CHECK_RES(vkQueueSubmit(vulkan->queue, 1, &submit_info, VK_NULL_HANDLE));
    VK_CHECK_RES(vkQueueWaitIdle(vulkan->queue));

    // Name
    if (image_name != NULL)
    {
        VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)*image, image_name, vulkan->vkSetDebugUtilsObjectNameEXT);
    }
    if (image_memory_name != NULL)
    {
        VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t)*image_memory, image_memory_name, vulkan->vkSetDebugUtilsObjectNameEXT);
    }
    if (image_view_name != NULL)
    {
        VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)*image_view, image_view_name, vulkan->vkSetDebugUtilsObjectNameEXT);
    }
}

void VulkanCreateSampler(vulkan_context_t* vulkan, VkFilter sampler_min_filter, VkFilter sampler_mag_filter, VkSampler* sampler, const char* sampler_name)
{
    assert(vulkan != NULL);
    assert(*sampler == VK_NULL_HANDLE);

    // Create sampler
    VkSamplerCreateInfo sampler_info;
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = NULL;
    sampler_info.flags = 0;
    sampler_info.minFilter = sampler_min_filter;
    sampler_info.magFilter = sampler_mag_filter;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 0.0f;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_NEVER;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    VK_CHECK_RES(vkCreateSampler(vulkan->device, &sampler_info, NULL, sampler));

    // Name
    if (sampler_name != NULL)
    {
        VulkanSetObjectName(vulkan->device, VK_OBJECT_TYPE_SAMPLER, (uint64_t)*sampler, sampler_name, vulkan->vkSetDebugUtilsObjectNameEXT);
    }
}


void VulkanCmdBeginDebugUtilsLabel(vulkan_context_t* vulkan, VkCommandBuffer command_buffer, const char* debug_label_name)
{
    VkDebugUtilsLabelEXT debug_label;
    debug_label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    debug_label.pNext = NULL;
    debug_label.color[0] = 0.0f;
    debug_label.color[1] = 0.0f;
    debug_label.color[2] = 0.0f;
    debug_label.color[3] = 0.0f;
    debug_label.pLabelName = debug_label_name;
    vulkan->vkCmdBeginDebugUtilsLabelEXT(command_buffer, &debug_label);
}

void VulkanCmdEndDebugUtilsLabel(vulkan_context_t* vulkan, VkCommandBuffer command_buffer)
{
    vulkan->vkCmdEndDebugUtilsLabelEXT(command_buffer);
}

void VulkanCmdTransitionImageLayout(vulkan_context_t* vulkan, VkCommandBuffer command_buffer, VkImage image, VkImageLayout image_old_layout, VkImageLayout image_new_layout, VkImageAspectFlags image_aspect, VkPipelineStageFlags barrier_src_pipeline_stage, VkAccessFlags barrier_src_access_mask, VkPipelineStageFlags barrier_dst_pipeline_stage, VkAccessFlags barrier_dst_access_mask, VkDependencyFlags barrier_dependency)
{
    VkImageMemoryBarrier image_barrier;
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.pNext = NULL;
    image_barrier.srcAccessMask = barrier_src_access_mask;
    image_barrier.dstAccessMask = barrier_dst_access_mask;
    image_barrier.oldLayout = image_old_layout;
    image_barrier.newLayout = image_new_layout;
    image_barrier.srcQueueFamilyIndex = vulkan->queue_index;
    image_barrier.dstQueueFamilyIndex = vulkan->queue_index;
    image_barrier.image = image;
    image_barrier.subresourceRange.aspectMask = image_aspect;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(command_buffer, barrier_src_pipeline_stage, barrier_dst_pipeline_stage, barrier_dependency, 0, NULL, 0, NULL, 1, &image_barrier);
}