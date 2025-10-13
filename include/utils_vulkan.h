#pragma once
#include <array>
#include <functional>
#include <vector>

#define VK_NO_PROTOTYPES
#include <volk.h>
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>

#define VK_CHECK(value) CHECK(value == VK_SUCCESS, __FILE__, __LINE__);
#define VK_CHECK_RET(value) if ( value != VK_SUCCESS ) { CHECK(false, __FILE__, __LINE__); return value; }
#define BL_CHECK(value) CHECK(value, __FILE__, __LINE__);

struct VulkanInstance 
{
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT messenger;
	VkDebugReportCallbackEXT reportCallback;
};

struct VulkanRenderDevice 
{
	uint32_t framebufferWidth;
	uint32_t framebufferHeight;

	VkDevice device;
	VkQueue graphicsQueue;
	VkPhysicalDevice physicalDevice;

	uint32_t graphicsFamily;

	VkSwapchainKHR swapchain;
	VkSemaphore semaphore;
	VkSemaphore renderSemaphore;

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	

	bool useCompute = false;

	
	uint32_t computeFamily;
	VkQueue computeQueue;

	
	std::vector<uint32_t> deviceQueueIndices;
	std::vector<VkQueue> deviceQueues;

	VkCommandBuffer computeCommandBuffer;
	VkCommandPool computeCommandPool;
};

struct VulkanContextFeatures
{
	bool supportScreenshots_ = false;

	bool geometryShader_ = true;
	bool tessellationShader_ = false;

	bool vertexPipelineStoresAndAtomics_ = false;
	bool fragmentStoresAndAtomics_ = false;
};

struct SwapchainSupportDetails final
{
	VkSurfaceCapabilitiesKHR capabilities = {};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct ShaderModule final
{
	std::vector<unsigned int> SPIRV;
	VkShaderModule shaderModule = nullptr;
};

struct VulkanBuffer
{
	VkBuffer       buffer;
	VkDeviceSize   size;
	VkDeviceMemory memory;

	/* Permanent mapping to CPU address space (see VulkanResources::addBuffer) */
	void* ptr;
};

struct VulkanImage final
{
	VkImage image = nullptr;
	VkDeviceMemory imageMemory = nullptr;
	VkImageView imageView = nullptr;
};

// Aggregate structure for passing around the texture data
struct VulkanTexture final
{
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	VkFormat format;

	VulkanImage image;
	VkSampler sampler;

	// Offscreen buffers require VK_IMAGE_LAYOUT_GENERAL && static textures have VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	VkImageLayout desiredLayout;
};
void CHECK(bool check, const char* fileName, int lineNumber);

bool setup_debug_callbacks(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, VkDebugReportCallbackEXT* reportCallback);

VkShaderStageFlagBits glslang_shader_stage_to_vulkan(glslang_stage_t sh);


size_t compile_shader_file(const char* file, ShaderModule& shaderModule);

VkResult create_shader_module(VkDevice device, ShaderModule* shader, const char* fileName);

inline VkPipelineShaderStageCreateInfo shader_stage_info(VkShaderStageFlagBits shaderStage, ShaderModule& module, const char* entryPoint)
{
	return VkPipelineShaderStageCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = shaderStage,
		.module = module.shaderModule,
		.pName = entryPoint,
		.pSpecializationInfo = nullptr,
	};
}

inline VkDescriptorSetLayoutBinding descriptor_set_layout_binding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1)
{
	return VkDescriptorSetLayoutBinding
	{
		.binding = binding,
		.descriptorType = descriptorType,
		.descriptorCount = descriptorCount,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr,
	};
}

inline VkWriteDescriptorSet buffer_write_descriptor_set(VkDescriptorSet ds, const VkDescriptorBufferInfo* bi, uint32_t bindIdx, VkDescriptorType dType)
{
	return VkWriteDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = ds,
		.dstBinding = bindIdx,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = dType,
		.pImageInfo = nullptr,
		.pBufferInfo = bi,
		.pTexelBufferView = nullptr
	};
}

inline VkWriteDescriptorSet image_write_descriptor_set(VkDescriptorSet ds, const VkDescriptorImageInfo* ii, uint32_t bindIdx)
{
	return VkWriteDescriptorSet{
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.pNext = nullptr,
	.dstSet = ds,
	.dstBinding = bindIdx,
	.dstArrayElement = 0,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	.pImageInfo = ii,
	.pBufferInfo = nullptr,
	.pTexelBufferView = nullptr
	};
}

void create_instance(VkInstance* instance);

VkResult create_device(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures deviceFeatures, uint32_t graphicsFamily, VkDevice* device);
VkResult create_device_with_compute(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures deviceFeatures, uint32_t graphicsFamily, uint32_t computeFamily, VkDevice* device);
VkResult create_swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t graphicsFamily, uint32_t width, uint32_t height, VkSwapchainKHR* swapchain, bool supportScreenshots = false);
SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface);
VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes);
uint32_t choose_swap_image_count(const VkSurfaceCapabilitiesKHR& capabilities);

size_t create_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, std::vector<VkImage>& swapchainImages, std::vector<VkImageView>& swapchainImageViews);
bool create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t layerCount = 1, uint32_t mipLevels = 1);
VkResult create_semaphore(VkDevice device, VkSemaphore* outSemaphore);

bool init_vulkan_render_device(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures deviceFeatures);
bool init_vulkan_render_device2(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures2 deviceFeatures2);
bool init_vulkan_render_device3(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, const VulkanContextFeatures& ctxFeatures = VulkanContextFeatures());
VkResult find_suitable_physical_device(VkInstance instance, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDevice* physicalDevice);

uint32_t find_queue_families(VkPhysicalDevice device, VkQueueFlags desiredFlags);
bool init_vulkan_render_device2_with_compute(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures2 deviceFeatures2, bool supportScreenshots);


VkResult create_device2_with_compute(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 deviceFeatures2, uint32_t graphicsFamily, uint32_t computeFamily, VkDevice* device);
bool is_device_suitable(VkPhysicalDevice device);

void destroy_vulkan_render_device(VulkanRenderDevice& vkDev);
void destroy_vulkan_instance(VulkanInstance& vk);


bool create_texture_sampler(VkDevice device, VkSampler* sampler, VkFilter minFilter = VK_FILTER_LINEAR, VkFilter maxFilter = VK_FILTER_LINEAR, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
bool create_descriptor_pool(VulkanRenderDevice& vkDev, uint32_t uniformBufferCount, uint32_t storageBufferCount, uint32_t samplerCount, VkDescriptorPool* descriptorPool);
VkFormat find_supported_format(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
uint32_t find_memory_type(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);
VkFormat find_depth_format(VkPhysicalDevice device);
bool has_stencil_component(VkFormat format);
bool create_graphics_pipeline(
	VulkanRenderDevice& vkDev,
	VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
	const std::vector<const char*>& shaderFiles,
	VkPipeline* pipeline,
	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST /* defaults to triangles*/,
	bool useDepth = true,
	bool useBlending = true,
	bool dynamicScissorState = false,
	int32_t customWidth = -1,
	int32_t customHeight = -1,
	uint32_t numPatchControlPoints = 0);

bool create_buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
bool create_shader_buffer(VulkanRenderDevice& vkDev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
bool create_uniform_buffer(VulkanRenderDevice& vkDev, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize bufferSize);
void upload_buffer_data(VulkanRenderDevice& vkDev, const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, const void* data, const size_t dataSize);
