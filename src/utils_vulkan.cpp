//#include <utils.h>
//#include <utils_vulkan.h>
//#include <bitmap.h>
//#include <utils_cubemap.h>
//#include <glslang/Public/resource_limits_c.h>
//#include <glslang/Public/ResourceLimits.h>
//
//#define VK_NO_PROTOTYPES
//#define GLFW_INCLUDE_VULKAN
//
//#include <GLFW/glfw3.h>
//
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
//#include <stb_image_resize2.h>
//
//#include <assimp/scene.h>
//#include <assimp/postprocess.h>
//#include <assimp/cimport.h>
//#include <assimp/version.h>
//
//#include <glm/glm.hpp>
//#include <glm/ext.hpp>
//
//#include <cstdio>
//#include <cstdlib>
//
//
//
//void CHECK(bool check, const char* fileName, int lineNumber)
//{
//	if (!check)
//	{
//		printf("CHECK() failed at %s:%i/n", fileName, lineNumber);
//		assert(false);
//		exit(EXIT_FAILURE);
//	}
//}
//
//static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
//	VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
//	VkDebugUtilsMessageTypeFlagsEXT Type,
//	const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
//	void* UserData
//)
//{
//	printf("Validation layer: %s\n", CallbackData->pMessage);
//	return VK_FALSE;
//}
//
//static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_report_callback(
//	VkDebugReportFlagsEXT      flags,
//	VkDebugReportObjectTypeEXT objectType,
//	uint64_t                   object,
//	size_t                     location,
//	int32_t                    messageCode,
//	const char* pLayerPrefix,
//	const char* pMessage,
//	void* UserData
//)
//{
//	// https://github.com/zeux/niagara/blob/master/src/device.cpp   [ignoring performance warnings]
//// silences warnings like "For optimal performance image layout should be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL instead of GENERAL."
//	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
//		return VK_FALSE;
//
//	printf("Debug callback (%s): %s\n", pLayerPrefix, pMessage);
//	return VK_FALSE;
//}
//
//bool setup_debug_callbacks(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, VkDebugReportCallbackEXT* reportCallback)
//{
//	{
//		const VkDebugUtilsMessengerCreateInfoEXT ci =
//		{
//			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
//			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
//			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
//			.pfnUserCallback = &vulkan_debug_callback,
//			.pUserData = nullptr
//		};
//
//		VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &ci, nullptr, messenger));
//	}
//	{
//		VkDebugReportCallbackCreateInfoEXT ci =
//		{
//			.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
//			.pNext = nullptr,
//			.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
//			.pfnCallback = &vulkan_debug_report_callback,
//			.pUserData = nullptr
//		};
//	}
//
//	return true;
//}
//
//size_t compile_shader_file(const char* file, ShaderModule& shaderModule)
//{
//	if (auto shaderSource = read_shader(file); !shaderSource.empty())
//		return compile_shader(glslang_shader_stage_from_filename(file), shaderSource.c_str(), shaderModule);
//	return 0;
//}
//
//
//
//VkResult create_shader_module(VkDevice device, ShaderModule* shader, const char* fileName)
//{
//	if (compile_shader_file(fileName, *shader) < 1)
//		return VK_NOT_READY;
//
//	const VkShaderModuleCreateInfo createInfo =
//	{
//		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
//		.codeSize = shader->SPIRV.size() * sizeof(unsigned int),
//		.pCode = shader->SPIRV.data(),
//	};
//
//	return vkCreateShaderModule(device, &createInfo, nullptr, &shader->shaderModule);
//}
//
//
//VkShaderStageFlagBits glslang_shader_stage_to_vulkan(glslang_stage_t sh)
//{
//	switch (sh)
//	{
//	case GLSLANG_STAGE_VERTEX:
//		return VK_SHADER_STAGE_VERTEX_BIT;
//	case GLSLANG_STAGE_FRAGMENT:
//		return VK_SHADER_STAGE_FRAGMENT_BIT;
//	case GLSLANG_STAGE_GEOMETRY:
//		return VK_SHADER_STAGE_GEOMETRY_BIT;
//	case GLSLANG_STAGE_TESSCONTROL:
//		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
//	case GLSLANG_STAGE_TESSEVALUATION:
//		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
//	case GLSLANG_STAGE_COMPUTE:
//		return VK_SHADER_STAGE_COMPUTE_BIT;
//	}
//
//	return VK_SHADER_STAGE_VERTEX_BIT;
//}
//
//glslang_stage_t glslang_shader_stage_from_filename(const char* fileName)
//{
//	if (ends_with(fileName, ".vert"))
//		return GLSLANG_STAGE_VERTEX;
//
//	if (ends_with(fileName, ".frag"))
//		return GLSLANG_STAGE_FRAGMENT;
//	
//	if (ends_with(fileName, ".geom"))
//		return GLSLANG_STAGE_GEOMETRY;
//
//	if (ends_with(fileName, ".comp"))
//		return GLSLANG_STAGE_COMPUTE;
//
//	if (ends_with(fileName, ".tesc"))
//		return GLSLANG_STAGE_TESSCONTROL;
//
//	if (ends_with(fileName, ".tese"))
//		return GLSLANG_STAGE_TESSEVALUATION;
//
//	return GLSLANG_STAGE_VERTEX;
//}
//
//static_assert(sizeof(TBuiltInResource) == sizeof(glslang_resource_t));
//
//static size_t compile_shader(glslang_stage_t stage, const char* shaderSource, ShaderModule& shaderModule)
//{
//	const glslang_input_t input =
//	{
//		.language = GLSLANG_SOURCE_GLSL,
//		.stage = stage,
//		.client = GLSLANG_CLIENT_VULKAN,
//		.client_version = GLSLANG_TARGET_VULKAN_1_1,
//		.target_language = GLSLANG_TARGET_SPV,
//		.target_language_version = GLSLANG_TARGET_SPV_1_3,
//		.code = shaderSource,
//		.default_version = 100,
//		.default_profile = GLSLANG_NO_PROFILE,
//		.force_default_version_and_profile = false,
//		.forward_compatible = false,
//		.messages = GLSLANG_MSG_DEFAULT_BIT,
//		.resource = glslang_default_resource(),
//	};
//
//	glslang_shader_t* shader = glslang_shader_create(&input);
//
//	if (!glslang_shader_preprocess(shader, &input))
//	{
//		fprintf(stderr, "GLSL preprocessing failed\n");
//		fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
//		fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
//		print_shader_source(input.code);
//		return 0;
//	}
//
//	if (!glslang_shader_parse(shader, &input))
//	{
//		fprintf(stderr, "GLSL parsing failed\n");
//		fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
//		fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
//		print_shader_source(glslang_shader_get_preprocessed_code(shader));
//		return 0;
//	}
//
//	glslang_program_t* program = glslang_program_create();
//	glslang_program_add_shader(program, shader);
//
//
//	if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
//	{
//		fprintf(stderr, "GLSL linking failed\n");
//		fprintf(stderr, "\n%s", glslang_program_get_info_log(program));
//		fprintf(stderr, "\n%s", glslang_program_get_info_debug_log(program));
//		return 0;
//	}
//
//	glslang_program_SPIRV_generate(program, stage);
//
//	shaderModule.SPIRV.resize(glslang_program_SPIRV_get_size(program));
//	glslang_program_SPIRV_get(program, shaderModule.SPIRV.data());
//
//	{
//		const char* spirv_messages =
//			glslang_program_SPIRV_get_messages(program);
//
//		if (spirv_messages)
//			fprintf(stderr, "%s", spirv_messages);
//	}
//
//	glslang_program_delete(program);
//	glslang_shader_delete(shader);
//
//	return shaderModule.SPIRV.size();
//}
//
//
//void create_instance(VkInstance* instance)
//{
//	const std::vector<const char*> validationLayers =
//	{
//		"VK_LAYER_KHRONOS_validation"
//	};
//
//	const std::vector<const char*> exts =
//	{
//		"VK_KHR_surface",
//		
//		#if defined (_WIN32)
//		"VK_KHR_win32_surface"
//		#endif
//
//		#if defined (__APPLE__)
//		"VK_MVK_macos_surface"
//		#endif
//
//		#if defined (__linux__)
//		"VK_KHR_xcb_surface"
//		#endif
//		, VK_EXT_DEBUG_UTILS_EXTENSION_NAME
//		, VK_EXT_DEBUG_REPORT_EXTENSION_NAME
//		//for indexed textures 
//		, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
//	};
//
//	const VkApplicationInfo appInfo =
//	{
//		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
//		.pNext = nullptr,
//		.pApplicationName = "Vulkan",
//		.applicationVersion = VK_MAKE_VERSION(1,0,0),
//		.pEngineName = "No Engine",
//		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
//		.apiVersion = VK_API_VERSION_1_1
//	};
//
//	const VkInstanceCreateInfo createInfo =
//	{
//				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.pApplicationInfo = &appInfo,
//		.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
//		.ppEnabledLayerNames = validationLayers.data(),
//		.enabledExtensionCount = static_cast<uint32_t>(exts.size()),
//		.ppEnabledExtensionNames = exts.data()
//	};
//
//	VK_CHECK(vkCreateInstance(&createInfo, nullptr, instance));
//
//	volkLoadInstance(*instance);
//}
//
//VkResult create_device(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures deviceFeatures, uint32_t graphicsFamily, VkDevice* device)
//{
//	const std::vector<const char*> extensions =
//	{
//		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
//		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
//	};
//
//	const float queuePriority = 1.0f;
//
//	const VkDeviceQueueCreateInfo qci =
//	{
//		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.queueFamilyIndex = graphicsFamily,
//		.queueCount = 1,
//		.pQueuePriorities = &queuePriority,
//
//	};
//
//
//	const VkDeviceCreateInfo ci =
//	{
//		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.queueCreateInfoCount = 1,
//		.pQueueCreateInfos = &qci,
//		.enabledLayerCount = 0,
//		.ppEnabledLayerNames = nullptr,
//		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
//		.ppEnabledExtensionNames = extensions.data(),
//		.pEnabledFeatures = &deviceFeatures
//	};
//
//	return vkCreateDevice(physicalDevice, &ci, nullptr, device);
//
//
//}
//
//VkResult create_device_with_compute(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures deviceFeatures, uint32_t graphicsFamily, uint32_t computeFamily, VkDevice* device)
//{
//	const std::vector<const char*> extensions =
//	{
//		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
//	};
//
//	if (graphicsFamily == computeFamily)
//		return create_device(physicalDevice, deviceFeatures, graphicsFamily, device);
//
//
//	const float queuePriorities[2] = { 0.0f, 0.0f };
//
//	const VkDeviceQueueCreateInfo gciGfx =
//	{
//		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.queueFamilyIndex = graphicsFamily,
//		.queueCount = 1,
//		.pQueuePriorities = &queuePriorities[0]
//	};
//
//	const VkDeviceQueueCreateInfo qciComp =
//	{
//		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.queueFamilyIndex = computeFamily,
//		.queueCount = 1,
//		.pQueuePriorities = &queuePriorities[1],
//	};
//
//	const VkDeviceQueueCreateInfo qci[2] = { gciGfx, qciComp };
//
//	const VkDeviceCreateInfo ci =
//	{
//		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.queueCreateInfoCount = 2,
//		.enabledLayerCount = 0,
//		.ppEnabledLayerNames = nullptr,
//		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
//		.ppEnabledExtensionNames = extensions.data(),
//		.pEnabledFeatures = &deviceFeatures,
//	};
//
//	return vkCreateDevice(physicalDevice, &ci, nullptr, device);
//}
//
//VkResult create_swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t graphicsFamily, uint32_t width, uint32_t height, VkSwapchainKHR* swapchain, bool supportScreenshots)
//{
//	auto swapchainSupport = query_swapchain_support(physicalDevice, surface);
//	auto surfaceFormat = choose_swap_surface_format(swapchainSupport.formats);
//	auto presentMode = choose_swap_present_mode(swapchainSupport.presentModes);
//
//	const VkSwapchainCreateInfoKHR ci =
//	{
//		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
//		.flags = 0,
//		.surface = surface,
//		.minImageCount = choose_swap_image_count(swapchainSupport.capabilities),
//		.imageFormat = surfaceFormat.format,
//		.imageColorSpace = surfaceFormat.colorSpace,
//		.imageExtent =
//		{
//			.width = width,
//			.height = height,
//		},
//		.imageArrayLayers = 1,
//		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | (supportScreenshots ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0u),
//		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
//		.queueFamilyIndexCount = 1,
//		.pQueueFamilyIndices = &graphicsFamily,
//		.preTransform = swapchainSupport.capabilities.currentTransform,
//		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
//		.presentMode = presentMode,
//		.clipped = VK_TRUE,
//		.oldSwapchain = VK_NULL_HANDLE,
//
//	};
//	
//	return vkCreateSwapchainKHR(device, &ci, nullptr, swapchain);
//
//}
//
//SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
//{
//	SwapchainSupportDetails details;
//	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
//
//	uint32_t formatCount;
//	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
//
//	if (formatCount)
//	{
//		details.formats.resize(formatCount);
//		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
//	}
//
//	uint32_t presentModeCount;
//	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
//
//	if (presentModeCount)
//	{
//		details.presentModes.resize(presentModeCount);
//		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
//	}
//
//	return details;
//
//}
//
//VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats)
//{
//	return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
//}
//
//VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes)
//{
//	for (const auto mode : availablePresentModes)
//		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
//			return mode;
//
//	
//	return VK_PRESENT_MODE_FIFO_KHR;
//}
//
//uint32_t choose_swap_image_count(const VkSurfaceCapabilitiesKHR& capabilities)
//{
//	const uint32_t imageCount = capabilities.minImageCount + 1;
//
//	const bool imageCountExceeded = capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount;
//
//	return imageCountExceeded ? capabilities.maxImageCount : imageCount;
//}
//
//size_t create_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& swapchainImages, std::vector<VkImageView>& swapchainImageViews)
//{
//	uint32_t imageCount = 0;
//	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
//
//	swapchainImages.resize(imageCount);
//	swapchainImageViews.resize(imageCount);
//
//	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));
//
//	for (unsigned i = 0; i < imageCount; i++)
//		if (!create_image_view(device, swapchainImages[i], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &swapchainImageViews[i]))
//			exit(0);
//
//	return static_cast<size_t>(imageCount);
//}
//
//bool create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView, VkImageViewType viewType, uint32_t layerCount, uint32_t mipLevels)
//{
//	const VkImageViewCreateInfo viewInfo =
//	{
//		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.image = image,
//		.viewType = viewType,
//		.format = format,
//		.subresourceRange =
//		{
//			.aspectMask = aspectFlags,
//			.baseMipLevel = 0,
//			.levelCount = mipLevels,
//			.baseArrayLayer = 0, 
//			.layerCount = layerCount
//		},
//
//	};
//
//	return (vkCreateImageView(device, &viewInfo, nullptr, imageView) == VK_SUCCESS);
//}
//
//VkResult create_semaphore(VkDevice device, VkSemaphore* outSemaphore)
//{
//	const VkSemaphoreCreateInfo ci =
//	{
//		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
//	};
//	
//	return vkCreateSemaphore(device, &ci, nullptr, outSemaphore);
//}
//
//bool init_vulkan_render_device(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures deviceFeatures)
//{
//	vkDev.framebufferHeight = height;
//	vkDev.framebufferWidth = width;
//
//
//	VK_CHECK(find_suitable_physical_device(vk.instance, selector, &vkDev.physicalDevice));
//	vkDev.graphicsFamily = find_queue_families(vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
//	VK_CHECK(create_device(vkDev.physicalDevice, deviceFeatures, vkDev.graphicsFamily, &vkDev.device));
//
//	vkGetDeviceQueue(vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue);
//	if (vkDev.graphicsQueue == nullptr)
//		exit(EXIT_FAILURE);
//
//	VkBool32 presentSupported = 0;
//	vkGetPhysicalDeviceSurfaceSupportKHR(vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported);
//	if (!presentSupported)
//		exit(EXIT_FAILURE);
//
//	VK_CHECK(create_swapchain(vkDev.device, vkDev.physicalDevice, vk.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain));
//	const size_t imageCount = create_swapchain_images(vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews);
//	vkDev.commandBuffers.resize(imageCount);
//
//	VK_CHECK(create_semaphore(vkDev.device, &vkDev.semaphore));
//	VK_CHECK(create_semaphore(vkDev.device, &vkDev.renderSemaphore));
//
//	const VkCommandPoolCreateInfo cpi =
//	{
//		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
//		.flags = 0,
//		.queueFamilyIndex = vkDev.graphicsFamily
//	};
//
//	VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi, nullptr, &vkDev.commandPool));
//
//	const VkCommandBufferAllocateInfo ai =
//	{
//		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
//		.pNext = nullptr,
//		.commandPool = vkDev.commandPool,
//		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
//		.commandBufferCount = static_cast<uint32_t>(vkDev.swapchainImages.size()),
//	};
//
//	VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai, &vkDev.commandBuffers[0]));
//	return true;
//
//}
//
//bool init_vulkan_render_device2(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures2 deviceFeatures2)
//{
//	vkDev.framebufferWidth = width;
//	vkDev.framebufferHeight = height;
//
//	VK_CHECK(find_suitable_physical_device(vk.instance, selector, &vkDev.physicalDevice));
//	vkDev.graphicsFamily = find_queue_families(vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
//	VK_CHECK(create_device2(vkDev.physicalDevice, deviceFeatures2, vkDev.graphicsFamily, &vkDev.device));
//
//	vkGetDeviceQueue(vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue);
//	if (vkDev.graphicsQueue == nullptr)
//		exit(EXIT_FAILURE);
//
//	VkBool32 presentSupported = 0;
//	vkGetPhysicalDeviceSurfaceSupportKHR(vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported);
//	if (!presentSupported)
//		exit(EXIT_FAILURE);
//
//	VK_CHECK(create_swapchain(vkDev.device, vkDev.physicalDevice, vk.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain));
//	const size_t imageCount = create_swapchain_images(vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews);
//	vkDev.commandBuffers.resize(imageCount);
//
//	VK_CHECK(create_semaphore(vkDev.device, &vkDev.semaphore));
//	VK_CHECK(create_semaphore(vkDev.device, &vkDev.renderSemaphore));
//
//	const VkCommandPoolCreateInfo cpi =
//	{
//		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
//		.flags = 0,
//		.queueFamilyIndex = vkDev.graphicsFamily
//	};
//
//	VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi, nullptr, &vkDev.commandPool));
//
//	const VkCommandBufferAllocateInfo ai =
//	{
//		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
//		.pNext = nullptr,
//		.commandPool = vkDev.commandPool,
//		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
//		.commandBufferCount = static_cast<uint32_t>(vkDev.swapchainImages.size()),
//	};
//
//	VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai, &vkDev.commandBuffers[0]));
//	return true;
//}
//
//bool init_vulkan_render_device3(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, const VulkanContextFeatures& ctxFeatures)
//{
//	VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures = {
//		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
//		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
//		.descriptorBindingVariableDescriptorCount = VK_TRUE,
//		.runtimeDescriptorArray = VK_TRUE,
//	};
//
//	VkPhysicalDeviceFeatures deviceFeatures = {
//		/* wireframe outlines */
//		.geometryShader = (VkBool32)(ctxFeatures.geometryShader_ ? VK_TRUE : VK_FALSE),
//		/* tesselation experiments */
//		.tessellationShader = (VkBool32)(ctxFeatures.tessellationShader_ ? VK_TRUE : VK_FALSE),
//		/* indirect instanced rendering */
//		.multiDrawIndirect = VK_TRUE,
//		.drawIndirectFirstInstance = VK_TRUE,
//		/* OIT and general atomic operations */
//		.vertexPipelineStoresAndAtomics = (VkBool32)(ctxFeatures.vertexPipelineStoresAndAtomics_ ? VK_TRUE : VK_FALSE),
//		.fragmentStoresAndAtomics = (VkBool32)(ctxFeatures.fragmentStoresAndAtomics_ ? VK_TRUE : VK_FALSE),
//		/* arrays of textures */
//		.shaderSampledImageArrayDynamicIndexing = VK_TRUE,
//		/* GL <-> VK material shader compatibility */
//		.shaderInt64 = VK_TRUE,
//	};
//
//	VkPhysicalDeviceFeatures2 deviceFeatures2 = {
//		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
//		.pNext = &physicalDeviceDescriptorIndexingFeatures,
//		.features = deviceFeatures  /*  */
//	};
//
//	return init_vulkan_render_device2_with_compute(vk, vkDev, width, height, is_device_suitable, deviceFeatures2, ctxFeatures.supportScreenshots_);
//}
//
//VkResult find_suitable_physical_device(VkInstance instance, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDevice* physicalDevice)
//{
//	uint32_t deviceCount = 0;
//	VK_CHECK_RET(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
//
//	if (!deviceCount) return VK_ERROR_INITIALIZATION_FAILED;
//
//	std::vector<VkPhysicalDevice> devices(deviceCount);
//	VK_CHECK_RET(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));
//
//	for (const auto& device : devices)
//	{
//		if (selector(device))
//		{
//			*physicalDevice = device;
//			return VK_SUCCESS;
//		}
//	}
//
//	return VK_ERROR_INITIALIZATION_FAILED;
//}
//
//uint32_t find_queue_families(VkPhysicalDevice device, VkQueueFlags desiredFlags)
//{
//	uint32_t familyCount = 0;
//	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
//
//	std::vector<VkQueueFamilyProperties> families(familyCount);
//	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());
//
//	for (int32_t i = 0; i != families.size(); i++)
//	{
//		if (families[i].queueCount > 0 && families[i].queueFlags & desiredFlags)
//			return i;
//	}
//
//	return 0;
//}
//bool init_vulkan_render_device2_with_compute(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures2 deviceFeatures2, bool supportScreenshots)
//{
//	vkDev.framebufferWidth = width;
//	vkDev.framebufferHeight = height;
//
//	VK_CHECK(find_suitable_physical_device(vk.instance, selector, &vkDev.physicalDevice));
//	vkDev.graphicsFamily = find_queue_families(vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
//	//	VK_CHECK(createDevice2(vkDev.physicalDevice, deviceFeatures2, vkDev.graphicsFamily, &vkDev.device));
//	//	VK_CHECK(vkGetBestComputeQueue(vkDev.physicalDevice, &vkDev.computeFamily));
//	vkDev.computeFamily = find_queue_families(vkDev.physicalDevice, VK_QUEUE_COMPUTE_BIT);
//	VK_CHECK(create_device2_with_compute(vkDev.physicalDevice, deviceFeatures2, vkDev.graphicsFamily, vkDev.computeFamily, &vkDev.device));
//
//	vkGetDeviceQueue(vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue);
//	if (vkDev.graphicsQueue == nullptr)
//		exit(EXIT_FAILURE);
//
//	vkGetDeviceQueue(vkDev.device, vkDev.computeFamily, 0, &vkDev.computeQueue);
//	if (vkDev.computeQueue == nullptr)
//		exit(EXIT_FAILURE);
//
//	VkBool32 presentSupported = 0;
//	vkGetPhysicalDeviceSurfaceSupportKHR(vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported);
//	if (!presentSupported)
//		exit(EXIT_FAILURE);
//
//	VK_CHECK(create_swapchain(vkDev.device, vkDev.physicalDevice, vk.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain, supportScreenshots));
//	const size_t imageCount = create_swapchain_images(vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews);
//	vkDev.commandBuffers.resize(imageCount);
//
//	VK_CHECK(create_semaphore(vkDev.device, &vkDev.semaphore));
//	VK_CHECK(create_semaphore(vkDev.device, &vkDev.renderSemaphore));
//
//	const VkCommandPoolCreateInfo cpi =
//	{
//		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
//		.flags = 0,
//		.queueFamilyIndex = vkDev.graphicsFamily
//	};
//
//	VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi, nullptr, &vkDev.commandPool));
//
//	const VkCommandBufferAllocateInfo ai =
//	{
//		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
//		.pNext = nullptr,
//		.commandPool = vkDev.commandPool,
//		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
//		.commandBufferCount = static_cast<uint32_t>(vkDev.swapchainImages.size()),
//	};
//
//	VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai, &vkDev.commandBuffers[0]));
//
//	{
//		// Create compute command pool
//		const VkCommandPoolCreateInfo cpi1 =
//		{
//			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
//			.pNext = nullptr,
//			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, /* Allow command from this pool buffers to be reset*/
//			.queueFamilyIndex = vkDev.computeFamily
//		};
//		VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi1, nullptr, &vkDev.computeCommandPool));
//
//		const VkCommandBufferAllocateInfo ai1 =
//		{
//			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
//			.pNext = nullptr,
//			.commandPool = vkDev.computeCommandPool,
//			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
//			.commandBufferCount = 1,
//		};
//
//		VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai1, &vkDev.computeCommandBuffer));
//	}
//
//	vkDev.useCompute = true;
//
//	return true;
//}
//bool init_vulkan_render_device_with_compute(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VkPhysicalDeviceFeatures deviceFeatures)
//{
//	vkDev.framebufferWidth = width;
//	vkDev.framebufferHeight = height;
//
//	VK_CHECK(find_suitable_physical_device(vk.instance, &is_device_suitable, &vkDev.physicalDevice));
//	vkDev.graphicsFamily = find_queue_families(vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
//	vkDev.computeFamily = find_queue_families(vkDev.physicalDevice, VK_QUEUE_COMPUTE_BIT);
//	//	VK_CHECK(vkGetBestComputeQueue(vkDev.physicalDevice, &vkDev.computeFamily));
//	VK_CHECK(create_device_with_compute(vkDev.physicalDevice, deviceFeatures, vkDev.graphicsFamily, vkDev.computeFamily, &vkDev.device));
//
//	vkDev.deviceQueueIndices.push_back(vkDev.graphicsFamily);
//	if (vkDev.graphicsFamily != vkDev.computeFamily)
//		vkDev.deviceQueueIndices.push_back(vkDev.computeFamily);
//
//	vkGetDeviceQueue(vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue);
//	if (vkDev.graphicsQueue == nullptr)
//		exit(EXIT_FAILURE);
//
//	vkGetDeviceQueue(vkDev.device, vkDev.computeFamily, 0, &vkDev.computeQueue);
//	if (vkDev.computeQueue == nullptr)
//		exit(EXIT_FAILURE);
//
//	VkBool32 presentSupported = 0;
//	vkGetPhysicalDeviceSurfaceSupportKHR(vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported);
//	if (!presentSupported)
//		exit(EXIT_FAILURE);
//
//	VK_CHECK(create_swapchain(vkDev.device, vkDev.physicalDevice, vk.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain));
//	const size_t imageCount = create_swapchain_images(vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews);
//	vkDev.commandBuffers.resize(imageCount);
//
//	VK_CHECK(create_semaphore(vkDev.device, &vkDev.semaphore));
//	VK_CHECK(create_semaphore(vkDev.device, &vkDev.renderSemaphore));
//
//	const VkCommandPoolCreateInfo cpi =
//	{
//		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
//		.flags = 0,
//		.queueFamilyIndex = vkDev.graphicsFamily
//	};
//
//	VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi, nullptr, &vkDev.commandPool));
//
//	const VkCommandBufferAllocateInfo ai =
//	{
//		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
//		.pNext = nullptr,
//		.commandPool = vkDev.commandPool,
//		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
//		.commandBufferCount = static_cast<uint32_t>(vkDev.swapchainImages.size()),
//	};
//
//	VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai, &vkDev.commandBuffers[0]));
//
//	{
//		// Create compute command pool
//		const VkCommandPoolCreateInfo cpi1 =
//		{
//			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
//			.pNext = nullptr,
//			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, /* Allow command from this pool buffers to be reset*/
//			.queueFamilyIndex = vkDev.computeFamily
//		};
//		VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi1, nullptr, &vkDev.computeCommandPool));
//
//		const VkCommandBufferAllocateInfo ai1 =
//		{
//			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
//			.pNext = nullptr,
//			.commandPool = vkDev.computeCommandPool,
//			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
//			.commandBufferCount = 1,
//		};
//
//		VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai1, &vkDev.computeCommandBuffer));
//	}
//
//	vkDev.useCompute = true;
//
//	return true;
//}
//
//bool is_device_suitable(VkPhysicalDevice device)
//{
//	VkPhysicalDeviceProperties deviceProperties;
//	vkGetPhysicalDeviceProperties(device, &deviceProperties);
//
//	VkPhysicalDeviceFeatures deviceFeatures;
//	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
//
//	const bool isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
//	const bool isIntegratedGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
//	const bool isGPU = isDiscreteGPU || isIntegratedGPU;
//	
//	return isGPU && deviceFeatures.geometryShader;
//}
//
//void destroy_vulkan_render_device(VulkanRenderDevice& vkDev)
//{
//	for (size_t i = 0; i < vkDev.swapchainImages.size(); i++)
//	{
//		vkDestroyImageView(vkDev.device, vkDev.swapchainImageViews[i], nullptr);
//	}
//
//	vkDestroySwapchainKHR(vkDev.device, vkDev.swapchain, nullptr);
//
//	vkDestroyCommandPool(vkDev.device, vkDev.commandPool, nullptr);
//
//	vkDestroySemaphore(vkDev.device, vkDev.semaphore, nullptr);
//	vkDestroySemaphore(vkDev.device, vkDev.renderSemaphore, nullptr);
//
//	if (vkDev.useCompute)
//	{
//		vkDestroyCommandPool(vkDev.device, vkDev.computeCommandPool, nullptr);
//	}
//
//	vkDestroyDevice(vkDev.device, nullptr);
//}
//
//void destroy_vulkan_instance(VulkanInstance& vk)
//{
//	vkDestroySurfaceKHR(vk.instance, vk.surface, nullptr);
//
//	vkDestroyDebugReportCallbackEXT(vk.instance, vk.reportCallback, nullptr);
//	vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.messenger, nullptr);
//
//	vkDestroyInstance(vk.instance, nullptr);
//}
//
//bool create_texture_sampler(VkDevice device, VkSampler* sampler, VkFilter minFilter, VkFilter maxFilter, VkSamplerAddressMode addressMode)
//{
//	const VkSamplerCreateInfo samplerInfo = {
//	.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
//	.pNext = nullptr,
//	.flags = 0,
//	.magFilter = VK_FILTER_LINEAR,
//	.minFilter = VK_FILTER_LINEAR,
//	.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
//	.addressModeU = addressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT,
//	.addressModeV = addressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT,
//	.addressModeW = addressMode, // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE VK_SAMPLER_ADDRESS_MODE_REPEAT,
//	.mipLodBias = 0.0f,
//	.anisotropyEnable = VK_FALSE,
//	.maxAnisotropy = 1,
//	.compareEnable = VK_FALSE,
//	.compareOp = VK_COMPARE_OP_ALWAYS,
//	.minLod = 0.0f,
//	.maxLod = 0.0f,
//	.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
//	.unnormalizedCoordinates = VK_FALSE
//	};
//
//	return (vkCreateSampler(device, &samplerInfo, nullptr, sampler) == VK_SUCCESS);
//}
//
//bool create_descriptor_pool(VulkanRenderDevice& vkDev, uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, VkDescriptorPool* descriptorPool)
//{
//	const uint32_t imageCount = static_cast<uint32_t> (vkDev.swapchainImages.size());
//
//	std::vector<VkDescriptorPoolSize> poolSizes;
//
//	if (uniformBufferCount)
//		poolSizes.emplace_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , .descriptorCount = imageCount * uniformBufferCount });
//
//	if (storageBufferCount)
//		poolSizes.emplace_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = imageCount * storageBufferCount });
//
//	if (samplerCount)
//		poolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = imageCount * samplerCount });
//
//	const VkDescriptorPoolCreateInfo poolInfo = 
//	{
//		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.maxSets = static_cast<uint32_t>(imageCount),
//		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
//		.pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data()
//	};
//
//	return (vkCreateDescriptorPool(vkDev.device, &poolInfo, nullptr, descriptorPool) == VK_SUCCESS);
//}
//
//VkFormat find_supported_format(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
//{
//	for (VkFormat format : candidates)
//	{
//		VkFormatProperties props;
//		vkGetPhysicalDeviceFormatProperties(device, format, &props);
//		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
//		{
//			return format;
//		}
//		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
//		{
//			return format;
//		}
//	}
//
//	printf("failed to find suported format!\n");
//	exit(0);
//}
//
//uint32_t find_memory_type(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
//{
//	VkPhysicalDeviceMemoryProperties memProps;
//	vkGetPhysicalDeviceMemoryProperties(device, &memProps);
//
//	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
//		if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) { //TODO : clean this
//			return i;
//		}
//	}
//
//	return 0xFFFFFFFF;
//}
//
//VkFormat find_depth_format(VkPhysicalDevice device)
//{
//	return find_supported_format(
//		device,
//		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
//		VK_IMAGE_TILING_OPTIMAL,
//		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
//	);
//}
//
//bool has_stencil_component(VkFormat format)
//{
//	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
//}
//
//bool create_graphics_pipeline(VulkanRenderDevice& vkDev, VkRenderPass renderPass, VkPipelineLayout pipelineLayout, const std::vector<const char*>& shaderFiles, VkPipeline* pipeline, VkPrimitiveTopology topology, bool useDepth, bool useBlending, bool dynamicScissorState, int32_t customWidth, int32_t customHeight, uint32_t numPatchControlPoints)
//{
//	std::vector<ShaderModule> shaderModules;
//	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
//
//	shaderStages.resize(shaderFiles.size());
//	shaderModules.resize(shaderFiles.size());
//
//	for (size_t i = 0; i < shaderFiles.size(); i++)
//	{
//		const char* file = shaderFiles[i];
//		VK_CHECK(create_shader_module(vkDev.device, &shaderModules[i], file));
//
//		VkShaderStageFlagBits stage = glslang_shader_stage_to_vulkan(glslang_shader_stage_from_filename(file));
//
//		shaderStages[i] = shader_stage_info(stage, shaderModules[i], "main");
//	}
//
//	const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
//	{
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
//	};
//
//	const VkPipelineInputAssemblyStateCreateInfo inputAssembly =
//	{
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
//		
//		.topology = topology,
//		.primitiveRestartEnable = VK_FALSE
//	};
//
//	const VkViewport viewport = 
//	{
//		.x = 0.0f,
//		.y = 0.0f,
//		.width = static_cast<float>(customWidth > 0 ? customWidth : vkDev.framebufferWidth),
//		.height = static_cast<float>(customHeight > 0 ? customHeight : vkDev.framebufferHeight),
//		.minDepth = 0.0f,
//		.maxDepth = 1.0f
//	};
//
//	const VkRect2D scissor = 
//	{
//		.offset = { 0, 0 },
//		.extent = { customWidth > 0 ? customWidth : vkDev.framebufferWidth, customHeight > 0 ? customHeight : vkDev.framebufferHeight }
//	};
//
//	const VkPipelineViewportStateCreateInfo viewportState = 
//	{
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
//		.viewportCount = 1,
//		.pViewports = &viewport,
//		.scissorCount = 1,
//		.pScissors = &scissor
//	};
//
//	const VkPipelineRasterizationStateCreateInfo rasterizer = 
//	{
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
//		.polygonMode = VK_POLYGON_MODE_FILL,
//		.cullMode = VK_CULL_MODE_NONE,
//		.frontFace = VK_FRONT_FACE_CLOCKWISE,
//		.lineWidth = 1.0f
//	};
//
//	const VkPipelineMultisampleStateCreateInfo multisampling =
//	{
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
//		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
//		.sampleShadingEnable = VK_FALSE,
//		.minSampleShading = 1.0f
//	};
//
//	const VkPipelineColorBlendAttachmentState colorBlendAttachment = 
//	{
//		.blendEnable = VK_TRUE,
//		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
//		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
//		.colorBlendOp = VK_BLEND_OP_ADD,
//		.srcAlphaBlendFactor = useBlending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
//		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
//		.alphaBlendOp = VK_BLEND_OP_ADD,
//		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
//	};
//
//	const VkPipelineColorBlendStateCreateInfo colorBlending = 
//	{
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
//		.logicOpEnable = VK_FALSE,
//		.logicOp = VK_LOGIC_OP_COPY,
//		.attachmentCount = 1,
//		.pAttachments = &colorBlendAttachment,
//		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
//	};
//
//	const VkPipelineDepthStencilStateCreateInfo depthStencil = 
//	{
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
//		.depthTestEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE),
//		.depthWriteEnable = static_cast<VkBool32>(useDepth ? VK_TRUE : VK_FALSE),
//		.depthCompareOp = VK_COMPARE_OP_LESS,
//		.depthBoundsTestEnable = VK_FALSE,
//		.minDepthBounds = 0.0f,
//		.maxDepthBounds = 1.0f
//	};
//
//	VkDynamicState dynamicStateElt = VK_DYNAMIC_STATE_SCISSOR;
//
//	const VkPipelineDynamicStateCreateInfo dynamicState =
//	{
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.dynamicStateCount = 1,
//		.pDynamicStates = &dynamicStateElt
//	};
//
//	const VkPipelineTessellationStateCreateInfo tessellationState = 
//	{
//		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.patchControlPoints = numPatchControlPoints
//	};
//
//	const VkGraphicsPipelineCreateInfo pipelineInfo =
//	{
//		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
//		.stageCount = static_cast<uint32_t>(shaderStages.size()),
//		.pStages = shaderStages.data(),
//		.pVertexInputState = &vertexInputInfo,
//		.pInputAssemblyState = &inputAssembly,
//		.pTessellationState = (topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) ? &tessellationState : nullptr,
//		.pViewportState = &viewportState,
//		.pRasterizationState = &rasterizer,
//		.pMultisampleState = &multisampling,
//		.pDepthStencilState = useDepth ? &depthStencil : nullptr,
//		.pColorBlendState = &colorBlending,
//		.pDynamicState = dynamicScissorState ? &dynamicState : nullptr,
//		.layout = pipelineLayout,
//		.renderPass = renderPass,
//		.subpass = 0,
//		.basePipelineHandle = VK_NULL_HANDLE,
//		.basePipelineIndex = -1
//	};
//
//	VK_CHECK(vkCreateGraphicsPipelines(vkDev.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline));
//
//	for (auto m : shaderModules)
//		vkDestroyShaderModule(vkDev.device, m.shaderModule, nullptr);
//
//	return true;
//
//}
//
//bool create_buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
//{
//	const VkBufferCreateInfo bufferInfo =
//	{
//		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.size = size, 
//		.usage = usage,
//		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
//		.queueFamilyIndexCount = 0, 
//		.pQueueFamilyIndices = nullptr,
//	};
//
//	VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));
//
//	VkMemoryRequirements memRequirements;
//	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
//
//	const VkMemoryAllocateInfo allocInfo = {
//		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
//		.pNext = nullptr,
//		.allocationSize = memRequirements.size,
//		.memoryTypeIndex = find_memory_type(physicalDevice, memRequirements.memoryTypeBits, properties),
//	};
//
//	VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));
//
//	vkBindBufferMemory(device, buffer, bufferMemory, 0);
//
//	return true;
//}
//
//bool create_shader_buffer(VulkanRenderDevice& vkDev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
//{
//	uint32_t familyCount = static_cast<uint32_t>(vkDev.deviceQueueIndices.size());
//
//	if (familyCount < 2)
//		return create_buffer(vkDev.device, vkDev.physicalDevice, size, usage, properties, buffer, bufferMemory);
//
//	const VkBufferCreateInfo bufferInfo = {
//		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.size = size,
//		.usage = usage,
//		.sharingMode = (familyCount > 1) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
//		.queueFamilyIndexCount = static_cast<uint32_t>(vkDev.deviceQueueIndices.size()),
//		.pQueueFamilyIndices = (familyCount > 1) ? vkDev.deviceQueueIndices.data() : nullptr
//	};
//
//	VK_CHECK(vkCreateBuffer(vkDev.device, &bufferInfo, nullptr, &buffer));
//
//	VkMemoryRequirements memRequirements;
//	vkGetBufferMemoryRequirements(vkDev.device, buffer, &memRequirements);
//
//	const VkMemoryAllocateInfo allocInfo = {
//		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
//		.pNext = nullptr,
//		.allocationSize = memRequirements.size,
//		.memoryTypeIndex = find_memory_type(vkDev.physicalDevice, memRequirements.memoryTypeBits, properties)
//	};
//
//	VK_CHECK(vkAllocateMemory(vkDev.device, &allocInfo, nullptr, &bufferMemory));
//
//	vkBindBufferMemory(vkDev.device, buffer, bufferMemory, 0);
//
//	return true;
//
//}
//
//bool create_uniform_buffer(VulkanRenderDevice& vkDev, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize bufferSize)
//{
//	return create_buffer(vkDev.device, vkDev.physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, bufferMemory);
//}
//
//void upload_buffer_data(VulkanRenderDevice& vkDev, const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, const void* data, const size_t dataSize)
//{
//	//EASY_FUNCTION();
//
//}
//
//
//
//
//
//
//VkResult create_device2_with_compute(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 deviceFeatures2, uint32_t graphicsFamily, uint32_t computeFamily, VkDevice* device)
//{
//	const std::vector<const char*> extensions =
//	{
//		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
//		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
//		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
//		// legacy drivers Vulkan 1.1
//		VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME
//	};
//
//	if (graphicsFamily == computeFamily)
//		return create_device2(physicalDevice, deviceFeatures2, graphicsFamily, device);
//
//	const float queuePriorities[2] = { 0.f, 0.f };
//	const VkDeviceQueueCreateInfo qciGfx =
//	{
//		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.queueFamilyIndex = graphicsFamily,
//		.queueCount = 1,
//		.pQueuePriorities = &queuePriorities[0]
//	};
//
//	const VkDeviceQueueCreateInfo qciComp =
//	{
//		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.queueFamilyIndex = computeFamily,
//		.queueCount = 1,
//		.pQueuePriorities = &queuePriorities[1]
//	};
//
//	const VkDeviceQueueCreateInfo qci[2] = { qciGfx, qciComp };
//
//	const VkDeviceCreateInfo ci =
//	{
//		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
//		.pNext = &deviceFeatures2,
//		.flags = 0,
//		.queueCreateInfoCount = 2,
//		.pQueueCreateInfos = qci,
//		.enabledLayerCount = 0,
//		.ppEnabledLayerNames = nullptr,
//		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
//		.ppEnabledExtensionNames = extensions.data(),
//		.pEnabledFeatures = nullptr
//	};
//
//	return vkCreateDevice(physicalDevice, &ci, nullptr, device);
//}
//VkResult create_device2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 deviceFeatures2, uint32_t graphicsFamily, VkDevice* device)
//{
//	const std::vector<const char*> extensions =
//	{
//		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
//		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
//		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
//		// for legacy drivers Vulkan 1.1
//		VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME
//	};
//
//	const float queuePriority = 1.0f;
//
//	const VkDeviceQueueCreateInfo qci =
//	{
//		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
//		.pNext = nullptr,
//		.flags = 0,
//		.queueFamilyIndex = graphicsFamily,
//		.queueCount = 1,
//		.pQueuePriorities = &queuePriority
//	};
//
//	const VkDeviceCreateInfo ci =
//	{
//		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
//		.pNext = &deviceFeatures2,
//		.flags = 0,
//		.queueCreateInfoCount = 1,
//		.pQueueCreateInfos = &qci,
//		.enabledLayerCount = 0,
//		.ppEnabledLayerNames = nullptr,
//		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
//		.ppEnabledExtensionNames = extensions.data(),
//		.pEnabledFeatures = nullptr
//	};
//
//	return vkCreateDevice(physicalDevice, &ci, nullptr, device);
//}
