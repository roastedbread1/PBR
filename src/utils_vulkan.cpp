#include <utils.h>
#include <utils_vulkan.h>
#include <bitmap.h>
#include <utils_cubemap.h>
#include <glslang/Public/resource_limits_c.h>
#include <glslang/Public/ResourceLimits.h>

#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_resize2.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <assimp/version.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <cstdio>
#include <cstdlib>



void CHECK(bool check, const char* fileName, int lineNumber)
{
	if (!check)
	{
		printf("CHECK() failed at %s:%i/n", fileName, lineNumber);
		assert(false);
		exit(EXIT_FAILURE);
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
	VkDebugUtilsMessageTypeFlagsEXT Type,
	const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
	void* UserData
)
{
	printf("Validation layer: %s\n", CallbackData->pMessage);
	return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_report_callback(
	VkDebugReportFlagsEXT      flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t                   object,
	size_t                     location,
	int32_t                    messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* UserData
)
{
	// https://github.com/zeux/niagara/blob/master/src/device.cpp   [ignoring performance warnings]
// silences warnings like "For optimal performance image layout should be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL instead of GENERAL."
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		return VK_FALSE;

	printf("Debug callback (%s): %s\n", pLayerPrefix, pMessage);
	return VK_FALSE;
}

bool setup_debug_callbacks(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, VkDebugReportCallbackEXT* reportCallback)
{
	{
		const VkDebugUtilsMessengerCreateInfoEXT ci =
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = &vulkan_debug_callback,
			.pUserData = nullptr
		};

		VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &ci, nullptr, messenger));
	}
	{
		VkDebugReportCallbackCreateInfoEXT ci =
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
			.pNext = nullptr,
			.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
			.pfnCallback = &vulkan_debug_report_callback,
			.pUserData = nullptr
		};
	}

	return true;
}

size_t compile_shader_file(const char* file, ShaderModule& shaderModule)
{
	if (auto shaderSource = read_shader_file(file); !shaderSource.empty())
		return compile_shader(glslang_shader_stage_from_filename(file), shaderSource.c_str(), shaderModule);
	return 0;
}



VkResult create_shader_module(VkDevice device, ShaderModule* shader, const char* fileName)
{
	if (compile_shader_file(fileName, *shader) < 1)
		return VK_NOT_READY;

	const VkShaderModuleCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = shader->SPIRV.size() * sizeof(unsigned int),
		.pCode = shader->SPIRV.data(),
	};

	return vkCreateShaderModule(device, &createInfo, nullptr, &shader->shaderModule);
}


VkShaderStageFlagBits glslang_shader_stage_to_vulkan(glslang_stage_t sh)
{
	switch (sh)
	{
	case GLSLANG_STAGE_VERTEX:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case GLSLANG_STAGE_FRAGMENT:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case GLSLANG_STAGE_GEOMETRY:
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	case GLSLANG_STAGE_TESSCONTROL:
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	case GLSLANG_STAGE_TESSEVALUATION:
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
	case GLSLANG_STAGE_COMPUTE:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	}

	return VK_SHADER_STAGE_VERTEX_BIT;
}

glslang_stage_t glslang_shader_stage_from_filename(const char* fileName)
{
	if (ends_with(fileName, ".vert"))
		return GLSLANG_STAGE_VERTEX;

	if (ends_with(fileName, ".frag"))
		return GLSLANG_STAGE_FRAGMENT;
	
	if (ends_with(fileName, ".geom"))
		return GLSLANG_STAGE_GEOMETRY;

	if (ends_with(fileName, ".comp"))
		return GLSLANG_STAGE_COMPUTE;

	if (ends_with(fileName, ".tesc"))
		return GLSLANG_STAGE_TESSCONTROL;

	if (ends_with(fileName, ".tese"))
		return GLSLANG_STAGE_TESSEVALUATION;

	return GLSLANG_STAGE_VERTEX;
}

static_assert(sizeof(TBuiltInResource) == sizeof(glslang_resource_t));

static size_t compile_shader(glslang_stage_t stage, const char* shaderSource, ShaderModule& shaderModule)
{ 


	const glslang_input_t input =
	{
		.language = GLSLANG_SOURCE_GLSL,
		.stage = stage,
		.client = GLSLANG_CLIENT_VULKAN,
		.client_version = GLSLANG_TARGET_VULKAN_1_4,
		.target_language = GLSLANG_TARGET_SPV,
		.target_language_version = GLSLANG_TARGET_SPV_1_5,
		.code = shaderSource,
		.default_version = 460,
		.default_profile = GLSLANG_CORE_PROFILE,
		.force_default_version_and_profile = false,
		.forward_compatible = false,
		.messages = GLSLANG_MSG_DEFAULT_BIT,
		.resource = glslang_default_resource(),
	};
	
	glslang_shader_t* shader = glslang_shader_create(&input);

	if (!glslang_shader_preprocess(shader, &input))
	{
		fprintf(stderr, "GLSL preprocessing failed\n");
		fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
		fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
		//print_shader_source(input.code);
		return 0;
	}

	if (!glslang_shader_parse(shader, &input))
	{
		fprintf(stderr, "GLSL parsing failed\n");
		fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
		fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
		//print_shader_source(glslang_shader_get_preprocessed_code(shader));
		return 0;
	}

	glslang_program_t* program = glslang_program_create();
	glslang_program_add_shader(program, shader);


	if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
	{
		fprintf(stderr, "GLSL linking failed\n");
		fprintf(stderr, "\n%s", glslang_program_get_info_log(program));
		fprintf(stderr, "\n%s", glslang_program_get_info_debug_log(program));
		return 0;
	}

	glslang_program_SPIRV_generate(program, stage);

	shaderModule.SPIRV.resize(glslang_program_SPIRV_get_size(program));
	glslang_program_SPIRV_get(program, shaderModule.SPIRV.data());

	{
		const char* spirv_messages =
			glslang_program_SPIRV_get_messages(program);

		if (spirv_messages)
			fprintf(stderr, "%s", spirv_messages);
	}

	glslang_program_delete(program);
	glslang_shader_delete(shader);

	return shaderModule.SPIRV.size();
}


void create_instance(VkInstance* instance)
{
	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> exts =
	{
		"VK_KHR_surface",
		
		#if defined (_WIN32)
		"VK_KHR_win32_surface"
		#endif

		#if defined (__APPLE__)
		"VK_MVK_macos_surface"
		#endif

		#if defined (__linux__)
		"VK_KHR_xcb_surface"
		#endif
		, VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		, VK_EXT_DEBUG_REPORT_EXTENSION_NAME
		//for indexed textures 
		, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
	};

	const VkApplicationInfo appInfo =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Vulkan",
		.applicationVersion = VK_MAKE_VERSION(1,0,0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_4
	};

	const VkInstanceCreateInfo createInfo =
	{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
		.ppEnabledLayerNames = validationLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(exts.size()),
		.ppEnabledExtensionNames = exts.data()
	};

	VK_CHECK(vkCreateInstance(&createInfo, nullptr, instance));

	volkLoadInstance(*instance);
}

VkResult create_device(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures deviceFeatures, uint32_t graphicsFamily, VkDevice* device)
{
	const std::vector<const char*> extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
	};

	const float queuePriority = 1.0f;

	const VkDeviceQueueCreateInfo qci =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority,

	};
	
	const VkDeviceCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &qci,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
		.pEnabledFeatures = &deviceFeatures
	};

	return vkCreateDevice(physicalDevice, &ci, nullptr, device);


}

VkResult create_device_with_compute(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures deviceFeatures, uint32_t graphicsFamily, uint32_t computeFamily, VkDevice* device)
{
	const std::vector<const char*> extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	if (graphicsFamily == computeFamily)
		return create_device(physicalDevice, deviceFeatures, graphicsFamily, device);


	const float queuePriorities[2] = { 0.0f, 0.0f };

	const VkDeviceQueueCreateInfo gciGfx =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriorities[0]
	};

	const VkDeviceQueueCreateInfo qciComp =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = computeFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriorities[1],
	};

	const VkDeviceQueueCreateInfo qci[2] = { gciGfx, qciComp };

	const VkDeviceCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 2,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
		.pEnabledFeatures = &deviceFeatures,
	};

	return vkCreateDevice(physicalDevice, &ci, nullptr, device);
}

VkResult create_swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t graphicsFamily, uint32_t width, uint32_t height, VkSwapchainKHR* swapchain, bool supportScreenshots)
{
	auto swapchainSupport = query_swapchain_support(physicalDevice, surface);
	auto surfaceFormat = choose_swap_surface_format(swapchainSupport.formats);
	auto presentMode = choose_swap_present_mode(swapchainSupport.presentModes);

	const VkSwapchainCreateInfoKHR ci =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.flags = 0,
		.surface = surface,
		.minImageCount = choose_swap_image_count(swapchainSupport.capabilities),
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent =
		{
			.width = width,
			.height = height,
		},
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | (supportScreenshots ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0u),
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &graphicsFamily,
		.preTransform = swapchainSupport.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,

	};
	
	return vkCreateSwapchainKHR(device, &ci, nullptr, swapchain);

}

	SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		SwapchainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;

	}

	

	void generate_mipmaps(
		VkPhysicalDevice physicalDevice,
		VkCommandBuffer commandBuffer,
		VkImage image,
		VkFormat imageFormat,
		int32_t texWidth,
		int32_t texHeight,
		uint32_t mipLevels,
		VkImageLayout currentLayout,
		VkImageLayout finalLayout)
	{
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			printf("Texture image format does not support linear blitting!\n");
			return;
		}
		else if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED)
		{
			printf("what the fuck");
			return;
		}
		VkImageMemoryBarrier barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};

		
		if (imageFormat == VK_FORMAT_D32_SFLOAT || imageFormat == VK_FORMAT_D32_SFLOAT_S8_UINT ||
			imageFormat == VK_FORMAT_D24_UNORM_S8_UINT || imageFormat == VK_FORMAT_D16_UNORM) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;


		barrier.subresourceRange.baseMipLevel = 0;
		barrier.oldLayout = currentLayout;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &barrier);

		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, 0, nullptr, 0, nullptr, 1, &barrier);

			// --- STEP 3: Blit i-1 -> i ---
			VkImageBlit blit = {};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource = { barrier.subresourceRange.aspectMask, i - 1, 0, 1 };
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource = { barrier.subresourceRange.aspectMask, i, 0, 1 };

			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit, VK_FILTER_LINEAR);

			barrier.subresourceRange.baseMipLevel = i;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, 0, nullptr, 0, nullptr, 1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}


		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = finalLayout;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &barrier);
	}


VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
}

VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto mode : availablePresentModes)
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;

	
	return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t choose_swap_image_count(const VkSurfaceCapabilitiesKHR& capabilities)
{
	const uint32_t imageCount = capabilities.minImageCount + 1;

	const bool imageCountExceeded = capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount;

	return imageCountExceeded ? capabilities.maxImageCount : imageCount;
}

size_t create_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& swapchainImages, std::vector<VkImageView>& swapchainImageViews)
{
	uint32_t imageCount = 0;
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));

	swapchainImages.resize(imageCount);
	swapchainImageViews.resize(imageCount);

	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));

	for (unsigned i = 0; i < imageCount; i++)
		if (!create_image_view(device, swapchainImages[i], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &swapchainImageViews[i]))
			exit(0);

	return static_cast<size_t>(imageCount);
}

bool create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView, VkImageViewType viewType, uint32_t layerCount, uint32_t mipLevels)
{
	const VkImageViewCreateInfo viewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = viewType,
		.format = format,
		.subresourceRange =
		{
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0, 
			.layerCount = layerCount
		},

	};

	return (vkCreateImageView(device, &viewInfo, nullptr, imageView) == VK_SUCCESS);
}

VkResult create_semaphore(VkDevice device, VkSemaphore* outSemaphore)
{
	const VkSemaphoreCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	
	return vkCreateSemaphore(device, &ci, nullptr, outSemaphore);
}

bool init_vulkan_render_device(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures deviceFeatures)
{
	vkDev.framebufferHeight = height;
	vkDev.framebufferWidth = width;


	VK_CHECK(find_suitable_physical_device(vk.instance, selector, &vkDev.physicalDevice));
	vkDev.graphicsFamily = find_queue_families(vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
	VK_CHECK(create_device(vkDev.physicalDevice, deviceFeatures, vkDev.graphicsFamily, &vkDev.device));

	vkGetDeviceQueue(vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue);
	if (vkDev.graphicsQueue == nullptr)
		exit(EXIT_FAILURE);

	VkBool32 presentSupported = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR(vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported);
	if (!presentSupported)
		exit(EXIT_FAILURE);

	VK_CHECK(create_swapchain(vkDev.device, vkDev.physicalDevice, vk.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain));
	const size_t imageCount = create_swapchain_images(vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews);
	vkDev.commandBuffers.resize(imageCount);

	VK_CHECK(create_semaphore(vkDev.device, &vkDev.semaphore));
	VK_CHECK(create_semaphore(vkDev.device, &vkDev.renderSemaphore));

	const VkCommandPoolCreateInfo cpi =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = 0,
		.queueFamilyIndex = vkDev.graphicsFamily
	};

	VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi, nullptr, &vkDev.commandPool));

	const VkCommandBufferAllocateInfo ai =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vkDev.commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<uint32_t>(vkDev.swapchainImages.size()),
	};

	VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai, &vkDev.commandBuffers[0]));
	return true;

}

bool init_vulkan_render_device2(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures2 deviceFeatures2)
{
	vkDev.framebufferWidth = width;
	vkDev.framebufferHeight = height;

	VK_CHECK(find_suitable_physical_device(vk.instance, selector, &vkDev.physicalDevice));
	vkDev.graphicsFamily = find_queue_families(vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
	VK_CHECK(create_device2(vkDev.physicalDevice, deviceFeatures2, vkDev.graphicsFamily, &vkDev.device));

	vkGetDeviceQueue(vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue);
	if (vkDev.graphicsQueue == nullptr)
		exit(EXIT_FAILURE);

	VkBool32 presentSupported = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR(vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported);
	if (!presentSupported)
		exit(EXIT_FAILURE);

	VK_CHECK(create_swapchain(vkDev.device, vkDev.physicalDevice, vk.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain));
	const size_t imageCount = create_swapchain_images(vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews);
	vkDev.commandBuffers.resize(imageCount);

	VK_CHECK(create_semaphore(vkDev.device, &vkDev.semaphore));
	VK_CHECK(create_semaphore(vkDev.device, &vkDev.renderSemaphore));

	const VkCommandPoolCreateInfo cpi =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = 0,
		.queueFamilyIndex = vkDev.graphicsFamily
	};

	VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi, nullptr, &vkDev.commandPool));

	const VkCommandBufferAllocateInfo ai =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vkDev.commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<uint32_t>(vkDev.swapchainImages.size()),
	};

	VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai, &vkDev.commandBuffers[0]));
	return true;
}

bool init_vulkan_render_device3(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, const VulkanContextFeatures& ctxFeatures)
{
	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
		.pNext = nullptr,
		.dynamicRendering = VK_TRUE
	};

	VkPhysicalDeviceBufferDeviceAddressFeatures bdaFeatures = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
		.pNext = &dynamicRenderingFeatures, 
		.bufferDeviceAddress = VK_TRUE
	};

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
		.pNext = &bdaFeatures,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
	};

	//shader draw param requirements
	VkPhysicalDeviceVulkan11Features features11 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.pNext = &physicalDeviceDescriptorIndexingFeatures, 
		.storageBuffer16BitAccess = VK_TRUE,
		.shaderDrawParameters = VK_TRUE,
	};
	VkPhysicalDeviceFeatures deviceFeatures = {
		/* wireframe outlines */
		.geometryShader = (VkBool32)(ctxFeatures.geometryShader_ ? VK_TRUE : VK_FALSE),
		/* tesselation experiments */
		.tessellationShader = (VkBool32)(ctxFeatures.tessellationShader_ ? VK_TRUE : VK_FALSE),
		/* indirect instanced rendering */
		.multiDrawIndirect = VK_TRUE,
		.drawIndirectFirstInstance = VK_TRUE,
		/* OIT and general atomic operations */
		.vertexPipelineStoresAndAtomics = (VkBool32)(ctxFeatures.vertexPipelineStoresAndAtomics_ ? VK_TRUE : VK_FALSE),
		.fragmentStoresAndAtomics = (VkBool32)(ctxFeatures.fragmentStoresAndAtomics_ ? VK_TRUE : VK_FALSE),
		/* arrays of textures */
		.shaderSampledImageArrayDynamicIndexing = VK_TRUE,
		/* GL <-> VK material shader compatibility */
		.shaderInt64 = VK_TRUE,
	};

	VkPhysicalDeviceFeatures2 deviceFeatures2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &features11,
		.features = deviceFeatures  /*  */
	};

	return init_vulkan_render_device2_with_compute(vk, vkDev, width, height, is_device_suitable, deviceFeatures2, ctxFeatures.supportScreenshots_);
}

VkResult find_suitable_physical_device(VkInstance instance, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDevice* physicalDevice)
{
	uint32_t deviceCount = 0;
	VK_CHECK_RET(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

	if (!deviceCount) return VK_ERROR_INITIALIZATION_FAILED;

	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_CHECK_RET(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

	for (const auto& device : devices)
	{
		if (selector(device))
		{
			*physicalDevice = device;
			return VK_SUCCESS;
		}
	}

	return VK_ERROR_INITIALIZATION_FAILED;
}

uint32_t find_queue_families(VkPhysicalDevice device, VkQueueFlags desiredFlags)
{
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);

	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

	for (int32_t i = 0; i != families.size(); i++)
	{
		if (families[i].queueCount > 0 && families[i].queueFlags & desiredFlags)
			return i;
	}

	return 0;
}
bool init_vulkan_render_device2_with_compute(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures2 deviceFeatures2, bool supportScreenshots)
{
	vkDev.framebufferWidth = width;
	vkDev.framebufferHeight = height;

	VK_CHECK(find_suitable_physical_device(vk.instance, selector, &vkDev.physicalDevice));
	vkDev.graphicsFamily = find_queue_families(vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
	//	VK_CHECK(createDevice2(vkDev.physicalDevice, deviceFeatures2, vkDev.graphicsFamily, &vkDev.device));
	//	VK_CHECK(vkGetBestComputeQueue(vkDev.physicalDevice, &vkDev.computeFamily));
	vkDev.computeFamily = find_queue_families(vkDev.physicalDevice, VK_QUEUE_COMPUTE_BIT);
	VK_CHECK(create_device2_with_compute(vkDev.physicalDevice, deviceFeatures2, vkDev.graphicsFamily, vkDev.computeFamily, &vkDev.device));
	volkLoadDevice(vkDev.device);
	vkGetDeviceQueue(vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue);
	if (vkDev.graphicsQueue == nullptr)
		exit(EXIT_FAILURE);

	vkGetDeviceQueue(vkDev.device, vkDev.computeFamily, 0, &vkDev.computeQueue);
	if (vkDev.computeQueue == nullptr)
		exit(EXIT_FAILURE);

	VkBool32 presentSupported = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR(vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported);
	if (!presentSupported)
		exit(EXIT_FAILURE);

	VK_CHECK(create_swapchain(vkDev.device, vkDev.physicalDevice, vk.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain, supportScreenshots));
	const size_t imageCount = create_swapchain_images(vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews);
	vkDev.commandBuffers.resize(imageCount);

	VK_CHECK(create_semaphore(vkDev.device, &vkDev.semaphore));
	VK_CHECK(create_semaphore(vkDev.device, &vkDev.renderSemaphore));

	const VkCommandPoolCreateInfo cpi =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = vkDev.graphicsFamily
	};

	VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi, nullptr, &vkDev.commandPool));

	const VkCommandBufferAllocateInfo ai =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vkDev.commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<uint32_t>(vkDev.swapchainImages.size()),
	};

	VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai, &vkDev.commandBuffers[0]));

	{
		// Create compute command pool
		const VkCommandPoolCreateInfo cpi1 =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, /* Allow command from this pool buffers to be reset*/
			.queueFamilyIndex = vkDev.computeFamily
		};
		VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi1, nullptr, &vkDev.computeCommandPool));

		const VkCommandBufferAllocateInfo ai1 =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = vkDev.computeCommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai1, &vkDev.computeCommandBuffer));
	}

	vkDev.useCompute = true;

	return true;
}

bool init_vulkan_render_device_with_compute(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VkPhysicalDeviceFeatures deviceFeatures)
{
	vkDev.framebufferWidth = width;
	vkDev.framebufferHeight = height;

	VK_CHECK(find_suitable_physical_device(vk.instance, &is_device_suitable, &vkDev.physicalDevice));
	vkDev.graphicsFamily = find_queue_families(vkDev.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
	vkDev.computeFamily = find_queue_families(vkDev.physicalDevice, VK_QUEUE_COMPUTE_BIT);
	//	VK_CHECK(vkGetBestComputeQueue(vkDev.physicalDevice, &vkDev.computeFamily));
	VK_CHECK(create_device_with_compute(vkDev.physicalDevice, deviceFeatures, vkDev.graphicsFamily, vkDev.computeFamily, &vkDev.device));

	vkDev.deviceQueueIndices.push_back(vkDev.graphicsFamily);
	if (vkDev.graphicsFamily != vkDev.computeFamily)
		vkDev.deviceQueueIndices.push_back(vkDev.computeFamily);

	vkGetDeviceQueue(vkDev.device, vkDev.graphicsFamily, 0, &vkDev.graphicsQueue);
	if (vkDev.graphicsQueue == nullptr)
		exit(EXIT_FAILURE);

	vkGetDeviceQueue(vkDev.device, vkDev.computeFamily, 0, &vkDev.computeQueue);
	if (vkDev.computeQueue == nullptr)
		exit(EXIT_FAILURE);

	VkBool32 presentSupported = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR(vkDev.physicalDevice, vkDev.graphicsFamily, vk.surface, &presentSupported);
	if (!presentSupported)
		exit(EXIT_FAILURE);

	VK_CHECK(create_swapchain(vkDev.device, vkDev.physicalDevice, vk.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain));
	const size_t imageCount = create_swapchain_images(vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews);
	vkDev.commandBuffers.resize(imageCount);

	VK_CHECK(create_semaphore(vkDev.device, &vkDev.semaphore));
	VK_CHECK(create_semaphore(vkDev.device, &vkDev.renderSemaphore));

	const VkCommandPoolCreateInfo cpi =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = 0,
		.queueFamilyIndex = vkDev.graphicsFamily
	};

	VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi, nullptr, &vkDev.commandPool));

	const VkCommandBufferAllocateInfo ai =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vkDev.commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<uint32_t>(vkDev.swapchainImages.size()),
	};

	VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai, &vkDev.commandBuffers[0]));

	{
		// create compute command pool
		const VkCommandPoolCreateInfo cpi1 =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, /* Allow command from this pool buffers to be reset*/
			.queueFamilyIndex = vkDev.computeFamily
		};
		VK_CHECK(vkCreateCommandPool(vkDev.device, &cpi1, nullptr, &vkDev.computeCommandPool));

		const VkCommandBufferAllocateInfo ai1 =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = vkDev.computeCommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		VK_CHECK(vkAllocateCommandBuffers(vkDev.device, &ai1, &vkDev.computeCommandBuffer));
	}

	vkDev.useCompute = true;

	return true;
}

bool is_device_suitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	const bool isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	const bool isIntegratedGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
	const bool isGPU = isDiscreteGPU || isIntegratedGPU;
	
	VkPhysicalDeviceBufferDeviceAddressFeatures bdaFeatures = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES
	};

	
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
		.pNext = &bdaFeatures 
	};

	
	VkPhysicalDeviceFeatures2 deviceFeatures2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &indexingFeatures 
	};

	
	vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);

	
	bool supportsGeometryShader = deviceFeatures2.features.geometryShader;
	bool supportsBufferDeviceAddress = bdaFeatures.bufferDeviceAddress;
	bool supportsDescriptorIndexing = indexingFeatures.runtimeDescriptorArray &&
		indexingFeatures.descriptorBindingVariableDescriptorCount &&
		indexingFeatures.shaderSampledImageArrayNonUniformIndexing;


	return supportsGeometryShader &&
		supportsBufferDeviceAddress &&
		supportsDescriptorIndexing;
}

void destroy_vulkan_render_device(VulkanRenderDevice& vkDev)
{
	for (size_t i = 0; i < vkDev.swapchainImages.size(); i++)
	{
		vkDestroyImageView(vkDev.device, vkDev.swapchainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(vkDev.device, vkDev.swapchain, nullptr);

	vkDestroyCommandPool(vkDev.device, vkDev.commandPool, nullptr);

	vkDestroySemaphore(vkDev.device, vkDev.semaphore, nullptr);
	vkDestroySemaphore(vkDev.device, vkDev.renderSemaphore, nullptr);

	if (vkDev.useCompute)
	{
		vkDestroyCommandPool(vkDev.device, vkDev.computeCommandPool, nullptr);
	}

	vkDestroyDevice(vkDev.device, nullptr);
}

void destroy_vulkan_instance(VulkanInstance& vk)
{
	vkDestroySurfaceKHR(vk.instance, vk.surface, nullptr);

	vkDestroyDebugReportCallbackEXT(vk.instance, vk.reportCallback, nullptr);
	vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.messenger, nullptr);

	vkDestroyInstance(vk.instance, nullptr);
}

bool create_texture_sampler(VkDevice device, VkSampler* sampler, VkFilter minFilter, VkFilter maxFilter, VkSamplerAddressMode addressMode)
{
	const VkSamplerCreateInfo samplerInfo = {
	.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	.pNext = nullptr,
	.flags = 0,
	.magFilter = maxFilter,
	.minFilter = minFilter,
	.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
	.addressModeU = addressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT,
	.addressModeV = addressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT,
	.addressModeW = addressMode, // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE VK_SAMPLER_ADDRESS_MODE_REPEAT,
	.mipLodBias = 0.0f,
	.anisotropyEnable = VK_FALSE,
	.maxAnisotropy = 1,
	.compareEnable = VK_FALSE,
	.compareOp = VK_COMPARE_OP_ALWAYS,
	.minLod = 0.0f,
	.maxLod = 16.0f,
	.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
	.unnormalizedCoordinates = VK_FALSE
	};

	return (vkCreateSampler(device, &samplerInfo, nullptr, sampler) == VK_SUCCESS);
}

bool create_descriptor_pool(VulkanRenderDevice& vkDev, uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, VkDescriptorPool* descriptorPool)
{
	const uint32_t imageCount = static_cast<uint32_t> (vkDev.swapchainImages.size());

	std::vector<VkDescriptorPoolSize> poolSizes;

	if (uniformBufferCount)
		poolSizes.emplace_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , .descriptorCount = imageCount * uniformBufferCount });

	if (storageBufferCount)
		poolSizes.emplace_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = imageCount * storageBufferCount });

	if (samplerCount)
		poolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = imageCount * samplerCount });

	const VkDescriptorPoolCreateInfo poolInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = static_cast<uint32_t>(imageCount),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data()
	};

	return (vkCreateDescriptorPool(vkDev.device, &poolInfo, nullptr, descriptorPool) == VK_SUCCESS);
}

VkFormat find_supported_format(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(device, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	printf("failed to find suported format!\n");
	exit(0);
}

uint32_t find_memory_type(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(device, &memProps);

	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) { //TODO : clean this
			return i;
		}
	}

	return 0xFFFFFFFF;
}

VkFormat find_depth_format(VkPhysicalDevice device)
{
	return find_supported_format(
		device,
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool has_stencil_component(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT ||
		format == VK_FORMAT_D16_UNORM_S8_UINT;
}

bool create_graphics_pipeline(
	VulkanRenderDevice& vkDev,
	VkPipelineLayout pipelineLayout,
	const std::vector<const char*>& shaderFiles,
	VkPipeline* pipeline,
	VkPrimitiveTopology topology,
	VkFormat colorFormat,
	VkFormat depthFormat,
	bool depthTestEnable,
	bool depthWriteEnable,
	bool useBlending,
	bool dynamicScissorState,
	VkSampleCountFlagBits numSamples,
	int32_t customWidth,
	int32_t customHeight,
	uint32_t numPatchControlPoints)
{
	std::vector<ShaderModule> shaderModules;
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	shaderStages.resize(shaderFiles.size());
	shaderModules.resize(shaderFiles.size());

	for (size_t i = 0; i < shaderFiles.size(); i++)
	{
		const char* file = shaderFiles[i];
		VK_CHECK(create_shader_module(vkDev.device, &shaderModules[i], file));

		VkShaderStageFlagBits stage = glslang_shader_stage_to_vulkan(glslang_shader_stage_from_filename(file));

		shaderStages[i] = shader_stage_info(stage, shaderModules[i], "main");
	}

	const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	};

	const VkPipelineInputAssemblyStateCreateInfo inputAssembly =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,

		.topology = topology,
		.primitiveRestartEnable = VK_FALSE
	};

	const VkViewport viewport =
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = static_cast<float>(customWidth > 0 ? customWidth : vkDev.framebufferWidth),
		.height = static_cast<float>(customHeight > 0 ? customHeight : vkDev.framebufferHeight),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	const VkRect2D scissor =
	{
		.offset = { 0, 0 },
		.extent = { (uint32_t)(customWidth > 0 ? customWidth : vkDev.framebufferWidth), (uint32_t)(customHeight > 0 ? customHeight : vkDev.framebufferHeight) } 
	};

	const VkPipelineViewportStateCreateInfo viewportState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	const VkPipelineRasterizationStateCreateInfo rasterizer =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f
	};

	const VkPipelineMultisampleStateCreateInfo multisampling =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = numSamples, 
		.sampleShadingEnable = VK_FALSE,    
		.minSampleShading = 1.0f
	};

	const VkPipelineColorBlendAttachmentState colorBlendAttachment =
	{
		.blendEnable = static_cast<VkBool32>(useBlending ? VK_TRUE : VK_FALSE), 
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = useBlending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	const VkPipelineColorBlendStateCreateInfo colorBlending =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
	};

	const VkPipelineDepthStencilStateCreateInfo depthStencil =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = static_cast<VkBool32>(depthTestEnable),     
		.depthWriteEnable = static_cast<VkBool32>(depthWriteEnable),   
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
	};

	VkDynamicState dynamicStateElt = VK_DYNAMIC_STATE_SCISSOR;


	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	const VkPipelineDynamicStateCreateInfo dynamicState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = (uint32_t)dynamicStates.size(),
		.pDynamicStates = dynamicStates.data()
	};
	const VkPipelineRenderingCreateInfo renderingCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &colorFormat,
		.depthAttachmentFormat = depthFormat,
		.stencilAttachmentFormat = has_stencil_component(depthFormat) ? depthFormat : VK_FORMAT_UNDEFINED,
	};

	const VkPipelineTessellationStateCreateInfo tessellationState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.patchControlPoints = numPatchControlPoints
	};

	const VkGraphicsPipelineCreateInfo pipelineInfo =
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &renderingCreateInfo,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),	
		.pStages = shaderStages.data(),
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pTessellationState = (topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) ? &tessellationState : nullptr,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		// Use depth/stencil state if either test or write is enabled
		.pDepthStencilState = (depthTestEnable || depthWriteEnable) ? &depthStencil : nullptr, 
		.pColorBlendState = &colorBlending,
		.pDynamicState = dynamicScissorState ? &dynamicState : nullptr,
		.layout = pipelineLayout,
		.renderPass = VK_NULL_HANDLE,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	VK_CHECK(vkCreateGraphicsPipelines(vkDev.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pipeline));

	for (auto m : shaderModules)
		vkDestroyShaderModule(vkDev.device, m.shaderModule, nullptr);

	return true;
}

bool create_buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	
	if (size == 0) {
		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;
		return false;
	}


	const VkBufferCreateInfo bufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size, 
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0, 
		.pQueueFamilyIndices = nullptr,
	};

	VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
	///
	VkMemoryAllocateFlags allocFlags = 0;
	if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		allocFlags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
	}
	VkMemoryAllocateFlagsInfo allocFlagsInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.pNext = nullptr,
		.flags = allocFlags,
	};


	const VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = &allocFlagsInfo,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = find_memory_type(physicalDevice, memRequirements.memoryTypeBits, properties),
	};

	VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));

	vkBindBufferMemory(device, buffer, bufferMemory, 0);

	return true;
}

bool create_shader_buffer(VulkanRenderDevice& vkDev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{


	if (size == 0) {
		buffer = VK_NULL_HANDLE;
		bufferMemory = VK_NULL_HANDLE;
		return false;
	}


	uint32_t familyCount = static_cast<uint32_t>(vkDev.deviceQueueIndices.size());

	if (familyCount < 2)
		return create_buffer(vkDev.device, vkDev.physicalDevice, size, usage, properties, buffer, bufferMemory);




	const VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = (familyCount > 1) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = static_cast<uint32_t>(vkDev.deviceQueueIndices.size()),
		.pQueueFamilyIndices = (familyCount > 1) ? vkDev.deviceQueueIndices.data() : nullptr
	};

	VK_CHECK(vkCreateBuffer(vkDev.device, &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vkDev.device, buffer, &memRequirements);
	
	VkMemoryAllocateFlags allocFlags = 0;
	if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		allocFlags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
	}

	VkMemoryAllocateFlagsInfo allocFlagsInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.pNext = nullptr,
		.flags = allocFlags,
	};

	const VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = &allocFlagsInfo,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = find_memory_type(vkDev.physicalDevice, memRequirements.memoryTypeBits, properties)
	};

	VK_CHECK(vkAllocateMemory(vkDev.device, &allocInfo, nullptr, &bufferMemory));

	vkBindBufferMemory(vkDev.device, buffer, bufferMemory, 0);

	return true;

}

bool create_uniform_buffer(VulkanRenderDevice& vkDev, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize bufferSize)
{
	return create_buffer(vkDev.device, vkDev.physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, bufferMemory);
}

void upload_buffer_data(VulkanRenderDevice& vkDev, const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, const void* data, const size_t dataSize)
{

	if (dataSize == 0) {
		return;
	}

	//EASY_FUNCTION();
	void* mappedData = nullptr;
	VK_CHECK(vkMapMemory(vkDev.device, bufferMemory, deviceOffset, dataSize, 0, &mappedData));	memcpy(mappedData, data, dataSize);
	vkUnmapMemory(vkDev.device, bufferMemory);
}

void download_buffer_data(VulkanRenderDevice& vkDev, const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, void* outData, const size_t dataSize)
{
	void* mappedData = nullptr;
	vkMapMemory(vkDev.device, bufferMemory, deviceOffset, dataSize, 0, &mappedData);
	memcpy(outData, mappedData, dataSize);
	vkUnmapMemory(vkDev.device, bufferMemory);
}

bool create_image(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageCreateFlags flags, uint32_t mipLevels)
{
	const VkImageCreateInfo imageInfo = {
	.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	.pNext = nullptr,
	.flags = flags,
	.imageType = VK_IMAGE_TYPE_2D,
	.format = format,
	.extent = VkExtent3D {.width = width, .height = height, .depth = 1 },
	.mipLevels = mipLevels,
	.arrayLayers = (uint32_t)((flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? 6 : 1),
	.samples = VK_SAMPLE_COUNT_1_BIT,
	.tiling = tiling,
	.usage = usage,
	.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	.queueFamilyIndexCount = 0,
	.pQueueFamilyIndices = nullptr,
	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	const VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = find_memory_type(physicalDevice, memRequirements.memoryTypeBits, properties)
	};

	VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));

	vkBindImageMemory(device, image, imageMemory, 0);
	return true;
}

bool create_volume(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t depth, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageCreateFlags flags)
{
	const VkImageCreateInfo imageInfo = {
	.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	.pNext = nullptr,
	.flags = flags,
	.imageType = VK_IMAGE_TYPE_3D,
	.format = format,
	.extent = VkExtent3D {.width = width, .height = height, .depth = depth },
	.mipLevels = 1,
	.arrayLayers = 1,
	.samples = VK_SAMPLE_COUNT_1_BIT,
	.tiling = tiling,
	.usage = usage,
	.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	.queueFamilyIndexCount = 0,
	.pQueueFamilyIndices = nullptr,
	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	const VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = find_memory_type(physicalDevice, memRequirements.memoryTypeBits, properties)
	};

	VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));

	vkBindImageMemory(device, image, imageMemory, 0);
	return true;
}

void render_pass_init_default(RenderPassCreateInfo* ci)
{
	ci->clearColor_ = false;
	ci->clearDepth_ = false;
	ci->flags_ = 0;
}

void render_pass_init(RenderPassCreateInfo* ci, bool clearColor, bool clearDepth, uint8_t flags)
{
	ci->clearColor_ = clearColor;
	ci->clearDepth_ = clearDepth;
	ci->flags_ = flags;
}

void render_pass_struct_init(RenderPass* rp)
{
	render_pass_init_default(&rp->info);
	rp->handle = VK_NULL_HANDLE;	
}

void render_pass_struct_destroy(RenderPass* rp)
{
}

bool create_color_only_render_pass(VulkanRenderDevice& vkDev, VkRenderPass* renderPass, const RenderPassCreateInfo& ci, VkFormat colorFormat)
{
	RenderPassCreateInfo ci2 = ci;
	ci2.clearDepth_ = false;
	return create_color_and_depth_render_pass(vkDev, false, renderPass, ci2, colorFormat);
}

bool create_color_and_depth_render_pass(VulkanRenderDevice& device, bool useDepth, VkRenderPass* renderPass, const RenderPassCreateInfo& ci, VkFormat colorFormat)
{
	const bool offscreenInt = ci.flags_ & eRenderPassBit_OffscreenInternal;
	const bool first = ci.flags_ & eRenderPassBit_First;
	const bool last  = ci.flags_ & eRenderPassBit_Last;	

	VkAttachmentDescription colorAttachment = {
		.flags = 0,
		.format = colorFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = offscreenInt ? VK_ATTACHMENT_LOAD_OP_LOAD : (ci.clearColor_ ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD),
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = (ci.clearColor_ || first) ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInt ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),//first ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInt ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
		.finalLayout = last ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	const VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentDescription depthAttachment = {
		.flags = 0,
		.format = useDepth ? find_depth_format(device.physicalDevice) : VK_FORMAT_D32_SFLOAT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = offscreenInt ? VK_ATTACHMENT_LOAD_OP_LOAD : (ci.clearDepth_ ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD),
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = ci.clearDepth_ ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInt ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	const VkAttachmentReference depthAttachmentRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	if (ci.flags_ & eRenderPassBit_Offscreen)
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	std::vector<VkSubpassDependency> dependencies = {
		/* VkSubpassDependency */ {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		}
	};

	if (ci.flags_ & eRenderPassBit_Offscreen)
	{
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

 		// Use subpass dependencies for layout transitions
 		dependencies.resize(2);
 
		dependencies[0] = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};

		dependencies[1] = {
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};
	}

	const VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = useDepth ? &depthAttachmentRef : nullptr,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
	};

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	const VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = static_cast<uint32_t>(useDepth ? 2 : 1),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.data()
	};

	return (vkCreateRenderPass(device.device, &renderPassInfo, nullptr, renderPass) == VK_SUCCESS);
}

bool create_depth_only_render_pass(VulkanRenderDevice& vkDev, VkRenderPass* renderPass, const RenderPassCreateInfo& ci)
{
	VkAttachmentDescription depthAttachment = {
		.flags = 0,
		.format = find_depth_format(vkDev.physicalDevice),
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = ci.clearDepth_ ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = ci.clearDepth_ ? VK_IMAGE_LAYOUT_UNDEFINED : (ci.flags_ & eRenderPassBit_OffscreenInternal ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	const VkAttachmentReference depthAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	std::vector<VkSubpassDependency> dependencies;

	if (ci.flags_ & eRenderPassBit_Offscreen)
	{
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// Use subpass dependencies for layout transitions
/* 		dependencies.resize(2);

		dependencies[0] = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, //VK_PIPELINE_STAGE_DEPTH_STENCIL_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};

		dependencies[1] = {
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_DEPTH_STENCIL_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};*/
	}

	const VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 0,
		.pColorAttachments = nullptr,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = &depthAttachmentRef,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
	};

	const VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = 1u,
		.pAttachments = &depthAttachment,
		.subpassCount = 1u,
		.pSubpasses = &subpass,
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.data()
	};

	return (vkCreateRenderPass(vkDev.device, &renderPassInfo, nullptr, renderPass) == VK_SUCCESS);
}

VkCommandBuffer begin_single_time_commands(VulkanRenderDevice& vkDev)
{
	VkCommandBuffer commandBuffer;

	const VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vkDev.commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	vkAllocateCommandBuffers(vkDev.device, &allocInfo, &commandBuffer);

	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void end_single_time_commands(VulkanRenderDevice& vkDev, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	const VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};

	vkQueueSubmit(vkDev.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vkDev.graphicsQueue);

	vkFreeCommandBuffers(vkDev.device, vkDev.commandPool, 1, &commandBuffer);

}

void copy_buffer(VulkanRenderDevice& vkDev, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{

	if (size == 0) return;

	VkCommandBuffer commandBuffer = begin_single_time_commands(vkDev);

	const VkBufferCopy copyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	end_single_time_commands(vkDev, commandBuffer);
}

void transition_image_layout(VulkanRenderDevice& vkDev, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount, uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = begin_single_time_commands(vkDev);

	transition_image_layout_cmd(commandBuffer, image, format, oldLayout, newLayout, layerCount, mipLevels);

	end_single_time_commands(vkDev, commandBuffer);
}

void transition_image_layout_cmd(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount, uint32_t mipLevels)
{
	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};

	// Handle Depth/Stencil aspect masks
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
		format == VK_FORMAT_D16_UNORM ||
		format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
		format == VK_FORMAT_D32_SFLOAT ||
		format == VK_FORMAT_S8_UINT ||
		format == VK_FORMAT_D16_UNORM_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (has_stencil_component(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	// --- Source Layout & Access Flags ---
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
		barrier.srcAccessMask = 0;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		barrier.srcAccessMask = 0; // No access needs to be waited on for present
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	else {
		// Fallback for unexpected layouts
		barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}

	// --- Destination Layout & Access Flags ---
	if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		barrier.dstAccessMask = 0;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else {
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
		destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}


bool create_color_and_depth_framebuffers(VulkanRenderDevice& vkDev, VkRenderPass renderPass, VkImageView depthImageView, std::vector<VkFramebuffer>& swapchainFramebuffers)
{
	swapchainFramebuffers.resize(vkDev.swapchainImageViews.size());

	for (size_t i = 0; i < vkDev.swapchainImages.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			vkDev.swapchainImageViews[i],
			depthImageView
		};

		const VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderPass = renderPass,
			.attachmentCount = static_cast<uint32_t>((depthImageView == VK_NULL_HANDLE) ? 1 : 2),
			.pAttachments = attachments.data(),
			.width = vkDev.framebufferWidth, 
			.height = vkDev.framebufferHeight,
			.layers = 1
		};

		VK_CHECK(vkCreateFramebuffer(vkDev.device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]));
	}

	return true;
}

bool create_color_and_depth_framebuffers(VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VkRenderPass renderPass, VkImageView colorImageView, VkImageView depthImageView, VkFramebuffer* framebuffer)
{
	std::array<VkImageView, 2> attachments = { colorImageView, depthImageView };

	const VkFramebufferCreateInfo framebufferInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = renderPass,
		.attachmentCount = (depthImageView == VK_NULL_HANDLE) ? 1u : 2u,
		.pAttachments = attachments.data(),
		.width = width,
		.height = height,
		.layers = 1
	};

	return (vkCreateFramebuffer(vkDev.device, &framebufferInfo, nullptr, framebuffer) == VK_SUCCESS);
}

bool create_depth_only_framebuffer(VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VkRenderPass renderPass, VkImageView depthImageView, VkFramebuffer* framebuffer)
{
	VkImageView attachment = depthImageView;

	const VkFramebufferCreateInfo framebufferInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = renderPass,
		.attachmentCount = 1u,
		.pAttachments = &attachment,
		.width = width, //TODO: verify
		.height = height,
		.layers = 1
	};

	return (vkCreateFramebuffer(vkDev.device, &framebufferInfo, nullptr, framebuffer) == VK_SUCCESS);
}

void copy_buffer_to_image(VulkanRenderDevice& vkDev, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount)
{
	VkCommandBuffer commandBuffer = begin_single_time_commands(vkDev);

	const VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = VkImageSubresourceLayers {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		},
		.imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0 },
		.imageExtent = VkExtent3D {.width = width, .height = height, .depth = 1 }
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	end_single_time_commands(vkDev, commandBuffer);
}

void copy_image_to_buffer(VulkanRenderDevice& vkDev, VkImage image, VkBuffer buffer, uint32_t width, uint32_t height, uint32_t layerCount)
{
	VkCommandBuffer commandBuffer =  begin_single_time_commands(vkDev);

	const VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = VkImageSubresourceLayers {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		},
		.imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0 },
		.imageExtent = VkExtent3D {.width = width, .height = height, .depth = 1 }
	};

	vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

	end_single_time_commands(vkDev, commandBuffer);
}

void copy_MIPbuffer_to_image(VulkanRenderDevice& vkDev, VkBuffer buffer, VkImage image, uint32_t mipLevels, uint32_t width, uint32_t height, uint32_t bytesPP, uint32_t layerCount)
{

	VkCommandBuffer commandBuffer = begin_single_time_commands(vkDev);

	uint32_t w = width, h = height;
	uint32_t offset = 0;
	std::vector<VkBufferImageCopy> regions(mipLevels);

	for (uint32_t i = 0; i < mipLevels; i++)
	{
		const VkBufferImageCopy region = {
			.bufferOffset = offset,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = VkImageSubresourceLayers {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = i,
				.baseArrayLayer = 0,
				.layerCount = layerCount
			},
			.imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0 },
			.imageExtent = VkExtent3D {.width = w, .height = h, .depth = 1 }
		};

		offset += w * h * layerCount * bytesPP;

		regions[i] = region;

		w >>= 1;
		h >>= 1;
	}

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(regions.size()),
		regions.data()
	);

	end_single_time_commands(vkDev, commandBuffer);
}

void destroy_vulkan_image(VkDevice device, VulkanImage& image)
{
	vkDestroyImageView(device, image.imageView, nullptr);
	vkDestroyImage(device, image.image, nullptr);
	vkFreeMemory(device, image.imageMemory, nullptr);
}

void destroy_vulkan_texture(VkDevice device, VulkanTexture& texture)
{
	destroy_vulkan_image(device, texture.image);
	vkDestroySampler(device, texture.sampler, nullptr);
}

uint32_t bytes_per_Tex_format(VkFormat fmt)
{
	switch (fmt)
	{
	case VK_FORMAT_R8_SINT:
	case VK_FORMAT_R8_UNORM:
		return 1;
	case VK_FORMAT_R16_SFLOAT:
		return 2;
	case VK_FORMAT_R16G16_SFLOAT:
		return 4;
	case VK_FORMAT_R16G16_SNORM:
		return 4;
	case VK_FORMAT_R8G8B8A8_SRGB:
	case VK_FORMAT_B8G8R8A8_SRGB:
	case VK_FORMAT_B8G8R8A8_UNORM:
		return 4;
	case VK_FORMAT_R8G8B8A8_UNORM:
		return 4;
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		return 4 * sizeof(uint16_t);
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return 4 * sizeof(float);
	default:
		break;
	}
	return 0;
}

bool update_texture_image(VulkanRenderDevice& vkDev, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, const void* imageData, VkImageLayout sourceImageLayout)
{
	uint32_t bytesPerPixel = bytes_per_Tex_format(texFormat);

	VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
	VkDeviceSize imageSize = layerSize * layerCount;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	create_buffer(vkDev.device, vkDev.physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	upload_buffer_data(vkDev, stagingBufferMemory, 0, imageData, imageSize);

	transition_image_layout(vkDev, textureImage, texFormat, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount);
	copy_buffer_to_image(vkDev, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), layerCount);
	transition_image_layout(vkDev, textureImage, texFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layerCount);

	vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
	vkFreeMemory(vkDev.device, stagingBufferMemory, nullptr);

	return true;
}
void copy_buffer_to_volume(VulkanRenderDevice& vkDev, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth)
{
	VkCommandBuffer commandBuffer = begin_single_time_commands(vkDev);

	const VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = VkImageSubresourceLayers {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0 },
		.imageExtent = VkExtent3D {.width = width, .height = height, .depth = depth }
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	end_single_time_commands(vkDev, commandBuffer);
}

bool update_texture_volume(VulkanRenderDevice& vkDev, VkImage& textureVolume, VkDeviceMemory& textureVolumeMemory, uint32_t texWidth, uint32_t texHeight, uint32_t texDepth, VkFormat texFormat, const void* volumeData, VkImageLayout sourceImageLayout)
{
	uint32_t bytesPerPixel = bytes_per_Tex_format(texFormat);

	VkDeviceSize volumeSize = texWidth * texHeight * texDepth * bytesPerPixel;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	create_buffer(vkDev.device, vkDev.physicalDevice, volumeSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	upload_buffer_data(vkDev, stagingBufferMemory, 0, volumeData, volumeSize);

	transition_image_layout(vkDev, textureVolume, texFormat, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
	copy_buffer_to_volume(vkDev, stagingBuffer, textureVolume, texWidth, texHeight, texDepth);
	transition_image_layout(vkDev, textureVolume, texFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

	vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
	vkFreeMemory(vkDev.device, stagingBufferMemory, nullptr);

	return true;
}

bool download_image_data(VulkanRenderDevice& vkDev, VkImage& textureImage, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, void* imageData, VkImageLayout sourceImageLayout)
{
	uint32_t bytesPerPixel = bytes_per_Tex_format(texFormat);

	VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
	VkDeviceSize imageSize = layerSize * layerCount;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	create_buffer(vkDev.device, vkDev.physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	transition_image_layout(vkDev, textureImage, texFormat, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, layerCount);
	copy_image_to_buffer(vkDev, textureImage, stagingBuffer, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), layerCount);
	transition_image_layout(vkDev, textureImage, texFormat, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sourceImageLayout, layerCount);

	download_buffer_data(vkDev, stagingBufferMemory, 0, imageData, imageSize);

	vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
	vkFreeMemory(vkDev.device, stagingBufferMemory, nullptr);

	return true;
}

bool create_depth_resources(VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VulkanImage& depth)
{
	VkFormat depthFormat = find_depth_format(vkDev.physicalDevice);

	if (!create_image(vkDev.device, vkDev.physicalDevice, width, height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth.image, depth.imageMemory))
		return false;

	if (!create_image_view(vkDev.device, depth.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &depth.imageView))
		return false;

	transition_image_layout(vkDev, depth.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	return true;
}

bool create_pipeline_layout(VkDevice device, VkDescriptorSetLayout dsLayout, VkPipelineLayout* pipelineLayout)
{
	const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &dsLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};

	return (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, pipelineLayout) == VK_SUCCESS);
}

bool create_pipeline_layout_with_constants(VkDevice device, VkDescriptorSetLayout dsLayout, VkPipelineLayout* pipelineLayout, uint32_t vtxConstSize, uint32_t fragConstSize)
{
	const VkPushConstantRange ranges[] =
	{
		{
			VK_SHADER_STAGE_VERTEX_BIT,   // stageFlags
			0,                            // offset
			vtxConstSize                  // size
		},

		{
			VK_SHADER_STAGE_FRAGMENT_BIT, // stageFlags
			vtxConstSize,                 // offset
			fragConstSize                 // size
		}
	};

	uint32_t constSize = (vtxConstSize > 0) + (fragConstSize > 0);

	const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &dsLayout,
		.pushConstantRangeCount = constSize,
		.pPushConstantRanges = (constSize == 0) ? nullptr :
			(vtxConstSize > 0 ? ranges : &ranges[1])
	};

	return (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, pipelineLayout) == VK_SUCCESS);
}

bool create_texture_image_from_data(VulkanRenderDevice& vkDev, VkImage& textureImage, VkDeviceMemory& textureImageMemory, void* imageData, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, VkImageCreateFlags flags)
{
	create_image(vkDev.device, vkDev.physicalDevice, texWidth, texHeight, texFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, flags);

	return update_texture_image(vkDev, textureImage, textureImageMemory, texWidth, texHeight, texFormat, layerCount, imageData);
}

bool create_MIP_texture_image_from_data(VulkanRenderDevice& vkDev, VkImage& textureImage, VkDeviceMemory& textureImageMemory, void* mipData, uint32_t mipLevels, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, VkImageCreateFlags flags)
{
	create_image(vkDev.device, vkDev.physicalDevice, texWidth, texHeight, texFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, flags, mipLevels);

	// now allocate staging buffer for all MIP levels
	uint32_t bytesPerPixel = bytes_per_Tex_format(texFormat);

	VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
	VkDeviceSize imageSize = layerSize * layerCount;

	uint32_t w = texWidth, h = texHeight;
	for (uint32_t i = 1; i < mipLevels; i++)
	{
		w >>= 1;
		h >>= 1;
		imageSize += w * h * bytesPerPixel * layerCount;
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	create_buffer(vkDev.device, vkDev.physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	upload_buffer_data(vkDev, stagingBufferMemory, 0, mipData, imageSize);

	transition_image_layout(vkDev, textureImage, texFormat, VK_IMAGE_LAYOUT_UNDEFINED/*sourceImageLayout*/, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount, mipLevels);
	copy_MIPbuffer_to_image(vkDev, stagingBuffer, textureImage, mipLevels, texWidth, texHeight, bytesPerPixel, layerCount);
	transition_image_layout(vkDev, textureImage, texFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layerCount, mipLevels);

	vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
	vkFreeMemory(vkDev.device, stagingBufferMemory, nullptr);

	return true;
}

bool create_texture_volume_from_data(VulkanRenderDevice& vkDev, VkImage& textureVolume, VkDeviceMemory& textureVolumeMemory, void* volumeData, uint32_t texWidth, uint32_t texHeight, uint32_t texDepth, VkFormat texFormat, VkImageCreateFlags flags)
{
	create_volume(vkDev.device, vkDev.physicalDevice, texWidth, texHeight, texDepth, texFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureVolume, textureVolumeMemory, flags);

	return update_texture_volume(vkDev, textureVolume, textureVolumeMemory, texWidth, texHeight, texDepth, texFormat, volumeData);
}

bool create_texture_image(VulkanRenderDevice& vkDev, const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* outTexWidth , uint32_t* outTexHeight , bool sRGB )
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels) {
		printf("Failed to load [%s] texture\n", filename); fflush(stdout);
		return false;
	}
	printf("DEBUG: Loaded %s. First pixel: %02X %02X %02X %02X\n", filename, pixels[0], pixels[1], pixels[2], pixels[3]);
	VkFormat format = sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

	bool result = create_texture_image_from_data(vkDev, textureImage, textureImageMemory,
		pixels, texWidth, texHeight, format);


	stbi_image_free(pixels);

	if (outTexWidth && outTexHeight) {
		*outTexWidth = (uint32_t)texWidth;
		*outTexHeight = (uint32_t)texHeight;
	}

	return result;


}

bool create_MIP_texture_image(VulkanRenderDevice& vkDev, const char* filename, uint32_t mipLevels, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width, uint32_t* height)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels) {
		printf("Failed to load [%s] texture\n", filename); fflush(stdout);
		return false;
	}

	uint32_t imgSize = texWidth * texHeight * texChannels;
	uint32_t mipSize = (imgSize * 3) >> 1;
	std::vector<uint8_t> mipData(mipSize);

	uint8_t* dst = mipData.data();
	uint8_t* src = dst;
	memcpy(dst, pixels, imgSize);

	uint32_t w = texWidth, h = texHeight;
	for (uint32_t i = 1; i < mipLevels; i++)
	{
		dst += (w * h * texChannels) >> 2;

		stbir_resize_uint8_linear(src, w, h, 0,
			dst, w / 2, h / 2, 0, (stbir_pixel_layout)texChannels);

		w >>= 1;
		h >>= 1;
		src = dst;
	}

	bool result = create_MIP_texture_image_from_data(vkDev, textureImage, textureImageMemory,
		mipData.data(), mipLevels, texWidth, texHeight,
		VK_FORMAT_R8G8B8A8_UNORM);

	stbi_image_free(pixels);

	if (width && height)
	{
		*width = texWidth;
		*height = texHeight;
	}

	return true;
}

static void float24to32(int w, int h, const float* img24, float* img32)
{
	const int numPixels = w * h;
	for (int i = 0; i != numPixels; i++)
	{
		*img32++ = *img24++;
		*img32++ = *img24++;
		*img32++ = *img24++;
		*img32++ = 1.0f;
	}
}
bool create_cube_texture_image(VulkanRenderDevice& vkDev, const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width, uint32_t* height)
{
	int w, h, comp;
	const float* img = stbi_loadf(filename, &w, &h, &comp, 3);
	std::vector<float> img32(w * h * 4);

	float24to32(w, h, img, img32.data());

	if (!img) {
		printf("Failed to load [%s] texture\n", filename); fflush(stdout);
		return false;
	}

	stbi_image_free((void*)img);

	Bitmap in;
	init_bitmap_from_data(&in, w, h, 4, eBitmapFormat_Float, img32.data());
	Bitmap out = convert_equirectangular_map_to_vertial_cross(in);

	Bitmap cube = convert_vertical_cross_to_cubemap_faces(out);

	if (width && height)
	{
		*width = w;
		*height = h;
	}

	return create_texture_image_from_data(vkDev, textureImage, textureImageMemory,
		cube.data.data(), cube.w, cube.h,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
}

bool create_MIP_cube_texture_image(VulkanRenderDevice& vkDev, const char* filename, uint32_t mipLevels, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width, uint32_t* height)
{
	int comp;
	int texWidth, texHeight;
	const float* img = stbi_loadf(filename, &texWidth, &texHeight, &comp, 3);

	if (!img) {
		printf("Failed to load [%s] texture\n", filename); fflush(stdout);
		return false;
	}

	uint32_t imageSize = texWidth * texHeight * 4;
	uint32_t mipSize = imageSize * 6;

	uint32_t w = texWidth, h = texHeight;
	for (uint32_t i = 1; i < mipLevels; i++)
	{
		imageSize = w * h * 4;
		w >>= 1;
		h >>= 1;
		mipSize += imageSize;
	}

	std::vector<float> mipData(mipSize);
	float* src = mipData.data();
	float* dst = mipData.data();

	w = texWidth;
	h = texHeight;
	float24to32(w, h, img, dst);

	for (uint32_t i = 1; i < mipLevels; i++)
	{
		imageSize = w * h * 4;
		dst += w * h * 4;
		stbir_resize(
			src, w, h, 0, dst, w / 2, h / 2, 0, STBIR_RGBA, STBIR_TYPE_FLOAT,
			STBIR_EDGE_WRAP, STBIR_FILTER_CUBICBSPLINE);

		w >>= 1;
		h >>= 1;
		src = dst;
	}

	src = mipData.data();
	dst = mipData.data();

	std::vector<float> mipCube(mipSize * 6);
	float* mip = mipCube.data();

	w = texWidth;
	h = texHeight;
	uint32_t faceSize = w / 4;
	for (uint32_t i = 0; i < mipLevels; i++)
	{
		Bitmap in;
		init_bitmap_from_data(&in,w, h, 4, eBitmapFormat_Float, src);
		Bitmap out = convert_equirectangular_map_to_vertial_cross(in);
		Bitmap cube = convert_vertical_cross_to_cubemap_faces(out);

		imageSize = faceSize * faceSize * 4;

		memcpy(mip, cube.data.data(), 6 * imageSize * sizeof(float));
		mip += imageSize * 6;

		src += w * h * 4;
		w >>= 1;
		h >>= 1;
	}

	stbi_image_free((void*)img);

	if (width && height)
	{
		*width = texWidth;
		*height = texHeight;
	}

	return create_MIP_texture_image_from_data(vkDev,
		textureImage, textureImageMemory,
		mipCube.data(), mipLevels, faceSize, faceSize,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
}

size_t allocate_vertex_buffer(VulkanRenderDevice& vkDev, VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t vertexDataSize, const void* vertexData, size_t indexDataSize, const void* indexData)
{
	VkDeviceSize bufferSize = vertexDataSize + indexDataSize;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	create_buffer(vkDev.device, vkDev.physicalDevice, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vkDev.device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertexData, vertexDataSize);
	memcpy((unsigned char*)data + vertexDataSize, indexData, indexDataSize);
	vkUnmapMemory(vkDev.device, stagingBufferMemory);

	create_buffer(vkDev.device, vkDev.physicalDevice, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *storageBuffer, *storageBufferMemory);

	copy_buffer(vkDev, stagingBuffer, *storageBuffer, bufferSize);

	vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
	vkFreeMemory(vkDev.device, stagingBufferMemory, nullptr);

	return bufferSize;
}

bool create_textured_vertex_buffer(VulkanRenderDevice& vkDev, const char* filename, VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t* vertexBufferSize, size_t* indexBufferSize)
{
	const aiScene* scene = aiImportFile(filename, aiProcess_Triangulate);

	if (!scene || !scene->HasMeshes())
	{
		printf("Unable to load %s\n", filename);
		exit(255);
	}

	const aiMesh* mesh = scene->mMeshes[0];
	struct VertexData
	{
		glm::vec3 pos;
		glm::vec2 tc;
	};

	std::vector<VertexData> vertices;
	for (unsigned i = 0; i != mesh->mNumVertices; i++)
	{
		const aiVector3D v = mesh->mVertices[i];
		const aiVector3D t = mesh->mTextureCoords[0][i];
		vertices.push_back({ .pos = glm::vec3(v.x, v.z, v.y), .tc = glm::vec2(t.x, 1.0f - t.y) });
	}

	std::vector<unsigned int> indices;
	for (unsigned i = 0; i != mesh->mNumFaces; i++)
	{
		for (unsigned j = 0; j != 3; j++)
			indices.push_back(mesh->mFaces[i].mIndices[j]);
	}
	aiReleaseImport(scene);

	*vertexBufferSize = sizeof(VertexData) * vertices.size();
	*indexBufferSize = sizeof(unsigned int) * indices.size();

	allocate_vertex_buffer(vkDev, storageBuffer, storageBufferMemory, *vertexBufferSize, vertices.data(), *indexBufferSize, indices.data());

	return true;
}

bool create_pbr_vertex_buffer(VulkanRenderDevice& vkDev, const char* filename, VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t* vertexBufferSize, size_t* indexBufferSize)
{
	const aiScene* scene = aiImportFile(filename, aiProcess_Triangulate);

	if (!scene || !scene->HasMeshes())
	{
		printf("Unable to load %s\n", filename);
		exit(255);
	}

	const aiMesh* mesh = scene->mMeshes[0];
	struct VertexData
	{
		glm::vec4 pos;
		glm::vec4 n;
		glm::vec4 tc;
	};

	std::vector<VertexData> vertices;
	for (unsigned i = 0; i != mesh->mNumVertices; i++)
	{
		const aiVector3D v = mesh->mVertices[i];
		const aiVector3D t = mesh->mTextureCoords[0][i];
		const aiVector3D n = mesh->mNormals[i];
		vertices.push_back({ .pos = glm::vec4(v.x, v.y, v.z, 1.0f), .n = glm::vec4(n.x, n.y, n.z, 0.0f), .tc = glm::vec4(t.x, 1.0f - t.y, 0.0f, 0.0f) });
	}

	std::vector<unsigned int> indices;
	for (unsigned i = 0; i != mesh->mNumFaces; i++)
	{
		for (unsigned j = 0; j != 3; j++)
			indices.push_back(mesh->mFaces[i].mIndices[j]);
	}
	aiReleaseImport(scene);

	*vertexBufferSize = sizeof(VertexData) * vertices.size();
	*indexBufferSize = sizeof(unsigned int) * indices.size();

	allocate_vertex_buffer(vkDev, storageBuffer, storageBufferMemory, *vertexBufferSize, vertices.data(), *indexBufferSize, indices.data());

	return true;
}


// For volumes use the call
// createImageView(device, image, /* whatever the format is */ VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, VkImageView* imageView, VK_IMAGE_VIEW_TYPE_3D, 1)




VkResult create_device2_with_compute(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 deviceFeatures2, uint32_t graphicsFamily, uint32_t computeFamily, VkDevice* device)
{
	const std::vector<const char*> extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		// legacy drivers Vulkan 1.1
		VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME

	};

	if (graphicsFamily == computeFamily)
		return create_device2(physicalDevice, deviceFeatures2, graphicsFamily, device);

	const float queuePriorities[2] = { 0.f, 0.f };
	const VkDeviceQueueCreateInfo qciGfx =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriorities[0]
	};

	const VkDeviceQueueCreateInfo qciComp =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = computeFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriorities[1]
	};

	const VkDeviceQueueCreateInfo qci[2] = { qciGfx, qciComp };

	const VkDeviceCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &deviceFeatures2,
		.flags = 0,
		.queueCreateInfoCount = 2,
		.pQueueCreateInfos = qci,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
		.pEnabledFeatures = nullptr
	};
	VkResult result = vkCreateDevice(physicalDevice, &ci, nullptr, device);
	if (result == VK_SUCCESS)
	{
		volkLoadDevice(*device); 
	}

	return result;
}
VkResult create_device2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 deviceFeatures2, uint32_t graphicsFamily, VkDevice* device)
{
	const std::vector<const char*> extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		// for legacy drivers Vulkan 1.1
		VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
	};

	const float queuePriority = 1.0f;

	const VkDeviceQueueCreateInfo qci =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
	};

	const VkDeviceCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &deviceFeatures2,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &qci,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
		.pEnabledFeatures = nullptr
	};

	VkResult result = vkCreateDevice(physicalDevice, &ci, nullptr, device);
	if (result == VK_SUCCESS)
	{
		volkLoadDevice(*device); 
	}
	return result;
}

bool execute_compute_shader(VulkanRenderDevice& vkDev, VkPipeline computePipeline, VkPipelineLayout pl, VkDescriptorSet ds, uint32_t xsize, uint32_t ysize, uint32_t zsize)
{
	VkCommandBuffer commandBuffer = vkDev.computeCommandBuffer;

	VkCommandBufferBeginInfo commandBufferBeginInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		0, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pl, 0, 1, &ds, 0, 0);

	vkCmdDispatch(commandBuffer, xsize, ysize, zsize);

	VkMemoryBarrier readoutBarrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_HOST_READ_BIT
	};

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &readoutBarrier, 0, nullptr, 0, nullptr);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		0, 0, 0, 0, 1, &commandBuffer, 0, 0
	};

	VK_CHECK(vkQueueSubmit(vkDev.computeQueue, 1, &submitInfo, 0));
	VK_CHECK(vkQueueWaitIdle(vkDev.computeQueue));

	return true;
}

bool create_compute_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout* descriptorSetLayout)
{
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] = {
		{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 },
		{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 }
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		0, 0, 2, descriptorSetLayoutBindings
	};

	return (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, 0, descriptorSetLayout) == VK_SUCCESS);
}

void insert_computed_buffer_barrier(VulkanRenderDevice& vkDev, VkCommandBuffer commandBuffer, VkBuffer buffer)
{
	uint32_t compute = vkDev.graphicsFamily;
	uint32_t graphics = vkDev.computeFamily;

	// make sure compute shader finishes before vertex shader reads vertices
	const VkBufferMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = compute,
		.dstQueueFamilyIndex = graphics,
		.buffer = buffer,
		.offset = 0,
		.size = VK_WHOLE_SIZE
	};

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
		0 /*VK_FLAGS_NONE*/, 0, nullptr, 1, &barrier, 0, nullptr);
}

void insert_computed_image_barrier(VkCommandBuffer commandBuffer, VkImage image)
{
	// make sure compute shader finishes before sampling
	const VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
		.newLayout = VK_IMAGE_LAYOUT_GENERAL,
		.image = image,
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0 /*VK_FLAGS_NONE*/, 0, nullptr, 0, nullptr, 1, &barrier);
}

VkSampleCountFlagBits get_max_usable_sample_count(VkPhysicalDevice physDevice)
{
	VkPhysicalDeviceProperties physDeviceProps;
	vkGetPhysicalDeviceProperties(physDevice, &physDeviceProps);

	VkSampleCountFlags counts = physDeviceProps.limits.framebufferColorSampleCounts & physDeviceProps.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

bool set_vk_object_name(VulkanRenderDevice& vkDev, void* object, VkObjectType objType, const char* name)
{
	VkDebugUtilsObjectNameInfoEXT nameInfo = {
	.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
	.pNext = nullptr,
	.objectType = objType,
	.objectHandle = (uint64_t)object,
	.pObjectName = name
	};

	return (vkSetDebugUtilsObjectNameEXT(vkDev.device, &nameInfo) == VK_SUCCESS);
}

void update_texture_in_descriptor_set_array(VulkanRenderDevice& vkDev, VkDescriptorSet ds, VulkanTexture t, uint32_t textureIndex, uint32_t bindingIdx)
{
	const VkDescriptorImageInfo imageInfo = {
		.sampler = t.sampler,
		.imageView = t.image.imageView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	VkWriteDescriptorSet writeSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = ds,
		.dstBinding = bindingIdx,
		.dstArrayElement = textureIndex,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &imageInfo
	};

	vkUpdateDescriptorSets(vkDev.device, 1, &writeSet, 0, nullptr);
}


