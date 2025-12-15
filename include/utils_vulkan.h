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
	uint32_t computeFamily;

	VkSwapchainKHR swapchain;
	VkSemaphore semaphore;
	VkSemaphore renderSemaphore;

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	



	VkQueue computeQueue;

	
	std::vector<uint32_t> deviceQueueIndices;
	std::vector<VkQueue> deviceQueues;

	VkCommandBuffer computeCommandBuffer;
	VkCommandPool computeCommandPool;
	bool useCompute = false;
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

static size_t compile_shader_file(const char* file, ShaderModule& shaderModule);
static size_t compile_shader(glslang_stage_t stage, const char* shaderSource, ShaderModule& shaderModule);
void CHECK(bool check, const char* fileName, int lineNumber);

bool setup_debug_callbacks(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, VkDebugReportCallbackEXT* reportCallback);

VkShaderStageFlagBits glslang_shader_stage_to_vulkan(glslang_stage_t sh);


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
bool create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, 
	uint32_t layerCount = 1, uint32_t mipLevels = 1);
VkResult create_semaphore(VkDevice device, VkSemaphore* outSemaphore);

bool init_vulkan_render_device(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures deviceFeatures);
bool init_vulkan_render_device2(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDeviceFeatures2 deviceFeatures2);
bool init_vulkan_render_device3(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, const VulkanContextFeatures& ctxFeatures = VulkanContextFeatures());
VkResult find_suitable_physical_device(VkInstance instance, std::function<bool(VkPhysicalDevice)> selector, VkPhysicalDevice* physicalDevice);

uint32_t find_queue_families(VkPhysicalDevice device, VkQueueFlags desiredFlags);
bool init_vulkan_render_device_with_compute(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VkPhysicalDeviceFeatures deviceFeatures);
bool init_vulkan_render_device2_with_compute(VulkanInstance& vk, VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, std::function<bool(VkPhysicalDevice)> selector, 
	VkPhysicalDeviceFeatures2 deviceFeatures2, bool supportScreenshots);

VkResult create_device2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 deviceFeatures2, uint32_t graphicsFamily, VkDevice* device);

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
//bool create_graphics_pipeline(
//	VulkanRenderDevice& vkDev,
//	VkRenderPass renderPass, VkPipelineLayout pipelineLayout,
//	const std::vector<const char*>& shaderFiles,
//	VkPipeline* pipeline,
//	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST /* defaults to triangles*/
//	bool useDepth = true,
//	bool useBlending = true,
//	bool dynamicScissorState = false,
//	int32_t customWidth = -1,
//	int32_t customHeight = -1,
//	uint32_t numPatchControlPoints = 0);
bool create_graphics_pipeline(
VulkanRenderDevice& vkDev,
	VkPipelineLayout pipelineLayout,
	const std::vector<const char*>& shaderFiles,
	VkPipeline* pipeline,
	VkPrimitiveTopology topology,
	VkFormat colorFormat,
	VkFormat depthFormat,
	bool depthTestEnable = true,
	bool depthWriteEnable = true,
	bool useBlending = true,
	bool dynamicScissorState = false,
	VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT,
	int32_t customWidth = -1,
	int32_t customHeight = -1,
	uint32_t numPatchControlPoints = 0);


bool create_buffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
bool create_shader_buffer(VulkanRenderDevice& vkDev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
bool create_uniform_buffer(VulkanRenderDevice& vkDev, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkDeviceSize bufferSize);
void upload_buffer_data(VulkanRenderDevice& vkDev, const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, const void* data, const size_t dataSize);
void download_buffer_data(VulkanRenderDevice& vkDev, const VkDeviceMemory& bufferMemory, VkDeviceSize deviceOffset, void* outData, const size_t dataSize);
bool create_image(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format,
	VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageCreateFlags flags = 0, uint32_t mipLevels = 1);
bool create_volume(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t depth,
	VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageCreateFlags flags);

typedef enum eRenderPassBit
{
	eRenderPassBit_First = 0x01,
	eRenderPassBit_Last = 0x02,
	eRenderPassBit_Offscreen = 0x04,
	eRenderPassBit_OffscreenInternal = 0x08,
} eRenderPassBit;

typedef struct RenderPassCreateInfo
{
	bool clearColor_;
	bool clearDepth_;
	uint8_t flags_;
} RenderPassCreateInfo;

typedef struct RenderPass
{
	RenderPassCreateInfo info;
	VkRenderPass handle;
} RenderPass;

void render_pass_init_default(RenderPassCreateInfo* ci);
void render_pass_init(RenderPassCreateInfo* ci, bool clearColor, bool clearDepth, uint8_t flags);

void render_pass_struct_init(RenderPass* rp);
void render_pass_struct_destroy(RenderPass* rp);

bool create_color_only_render_pass(VulkanRenderDevice& vkDev, VkRenderPass* renderPass, const RenderPassCreateInfo& ci, VkFormat colorFormat);
bool create_color_and_depth_render_pass(VulkanRenderDevice& device, bool useDepth, VkRenderPass* renderPass, const RenderPassCreateInfo& ci, VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM);
bool create_depth_only_render_pass(VulkanRenderDevice& vkDev, VkRenderPass* renderPass, const RenderPassCreateInfo& ci);

VkCommandBuffer begin_single_time_commands(VulkanRenderDevice& vkDev);
void end_single_time_commands(VulkanRenderDevice& vkDev, VkCommandBuffer commandBuffer);
void copy_buffer(VulkanRenderDevice& vkDev, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
void transition_image_layout(VulkanRenderDevice& vkDev, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount = 1, uint32_t mipLevels = 1);
void transition_image_layout_cmd(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount = 1, uint32_t mipLevels = 1);


bool create_color_and_depth_framebuffers(VulkanRenderDevice& vkDev, VkRenderPass renderPass, VkImageView depthImageView, std::vector<VkFramebuffer>& swapchainFramebuffers);
bool create_color_and_depth_framebuffers(VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VkRenderPass renderPass, VkImageView colorImageView, VkImageView depthImageView, VkFramebuffer* framebuffer);
bool create_depth_only_framebuffer(VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VkRenderPass renderPass, VkImageView depthImageView, VkFramebuffer* framebuffer);

void copy_buffer_to_image(VulkanRenderDevice& vkDev, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount = 1);
void copy_buffer_to_volume(VulkanRenderDevice& vkDev, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t depth);
void copy_image_to_buffer(VulkanRenderDevice& vkDev, VkImage image, VkBuffer buffer, uint32_t width, uint32_t height, uint32_t layerCount = 1);
void copy_MIPbuffer_to_image(VulkanRenderDevice& vkDev, VkBuffer buffer, VkImage image, uint32_t mipLevels, uint32_t width, uint32_t height, uint32_t bytesPP, uint32_t layerCount = 1);


void destroy_vulkan_image(VkDevice device, VulkanImage& image);
void destroy_vulkan_texture(VkDevice device, VulkanTexture& texture);

uint32_t bytes_per_Tex_format(VkFormat fmt);


/* VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for real update of an existing texture */
bool update_texture_image(VulkanRenderDevice& vkDev, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, 
	uint32_t layerCount, const void* imageData, VkImageLayout sourceImageLayout = VK_IMAGE_LAYOUT_UNDEFINED);
bool update_texture_volume(VulkanRenderDevice& vkDev, VkImage& textureVolume, VkDeviceMemory& textureVolumeMemory, uint32_t texWidth, uint32_t texHeight, uint32_t texDepth, 
	VkFormat texFormat, const void* volumeData, VkImageLayout sourceImageLayout = VK_IMAGE_LAYOUT_UNDEFINED);
bool download_image_data(VulkanRenderDevice& vkDev, VkImage& textureImage, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, void* imageData, VkImageLayout sourceImageLayout);
bool create_depth_resources(VulkanRenderDevice& vkDev, uint32_t width, uint32_t height, VulkanImage& depth);


bool create_pipeline_layout(VkDevice device, VkDescriptorSetLayout dsLayout, VkPipelineLayout* pipelineLayout);
bool create_pipeline_layout_with_constants(VkDevice device, VkDescriptorSetLayout dsLayout, VkPipelineLayout* pipelineLayout, uint32_t vtxConstSize, uint32_t fragConstSize);

bool create_texture_image_from_data(VulkanRenderDevice& vkDev, VkImage& textureImage, VkDeviceMemory& textureImageMemory, void* imageData, uint32_t texWidth, uint32_t texHeight, 
	VkFormat texFormat, 	uint32_t layerCount = 1, VkImageCreateFlags flags = 0);
bool create_MIP_texture_image_from_data(VulkanRenderDevice& vkDev, VkImage& textureImage, VkDeviceMemory& textureImageMemory, void* mipData, uint32_t mipLevels, uint32_t texWidth, uint32_t texHeight,
	VkFormat texFormat, uint32_t layerCount = 1, VkImageCreateFlags flags = 0);
bool create_texture_volume_from_data(VulkanRenderDevice& vkDev, VkImage& textureVolume, VkDeviceMemory& textureVolumeMemory, void* volumeData, uint32_t texWidth, uint32_t texHeight, uint32_t texDepth,
	VkFormat texFormat, VkImageCreateFlags flags = 0);

bool create_texture_image(VulkanRenderDevice& vkDev, const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* outTexWidth = nullptr, uint32_t* outTexHeight = nullptr, bool sRGB = false);
bool create_MIP_texture_image(VulkanRenderDevice& vkDev, const char* filename, uint32_t mipLevels, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width = nullptr, uint32_t* height = nullptr);
bool create_cube_texture_image(VulkanRenderDevice& vkDev, const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width = nullptr, uint32_t* height = nullptr);
bool create_MIP_cube_texture_image(VulkanRenderDevice& vkDev, const char* filename, uint32_t mipLevels, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width = nullptr, uint32_t* height = nullptr);

size_t allocate_vertex_buffer(VulkanRenderDevice& vkDev, VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t vertexDataSize, const void* vertexData, size_t indexDataSize, const void* indexData);

bool create_textured_vertex_buffer(VulkanRenderDevice& vkDev, const char* filename, VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t* vertexBufferSize, size_t* indexBufferSize);
bool create_pbr_vertex_buffer(VulkanRenderDevice& vkDev, const char* filename, VkBuffer* storageBuffer, VkDeviceMemory* storageBufferMemory, size_t* vertexBufferSize, size_t* indexBufferSize);
bool execute_compute_shader(VulkanRenderDevice& vkDev, VkPipeline computePipeline, VkPipelineLayout pl, VkDescriptorSet ds, uint32_t xsize, uint32_t ysize, uint32_t zsize);

bool create_compute_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout* descriptorSetLayout);

void insert_computed_buffer_barrier(VulkanRenderDevice& vkDev, VkCommandBuffer commandBuffer, VkBuffer buffer);
void insert_computed_image_barrier(VkCommandBuffer commandBuffer, VkImage image);

VkSampleCountFlagBits get_max_usable_sample_count(VkPhysicalDevice physDevice);

inline uint32_t getVulkanBufferAlignment(VulkanRenderDevice& vkDev)
{
	VkPhysicalDeviceProperties devProps;
	vkGetPhysicalDeviceProperties(vkDev.physicalDevice, &devProps);
	return static_cast<uint32_t>(devProps.limits.minStorageBufferOffsetAlignment);
}

inline bool is_depth_format(VkFormat fmt) {
	return
		(fmt == VK_FORMAT_D16_UNORM) ||
		(fmt == VK_FORMAT_X8_D24_UNORM_PACK32) ||
		(fmt == VK_FORMAT_D32_SFLOAT) ||
		(fmt == VK_FORMAT_D16_UNORM_S8_UINT) ||
		(fmt == VK_FORMAT_D24_UNORM_S8_UINT) ||
		(fmt == VK_FORMAT_D32_SFLOAT_S8_UINT);
}
bool set_vk_object_name(VulkanRenderDevice& vkDev, void* object, VkObjectType objType, const char* name);

inline bool setVkImageName(VulkanRenderDevice& vkDev, void* object, const char* name)
{
	return set_vk_object_name(vkDev, object, VK_OBJECT_TYPE_IMAGE, name);
}
/* This routine updates one texture discriptor in one descriptor set */

void update_texture_in_descriptor_set_array(VulkanRenderDevice& vkDev, VkDescriptorSet ds, VulkanTexture t, uint32_t textureIndex, uint32_t bindingIdx);
glslang_stage_t glslang_shader_stage_from_filename(const char* fileName);
//void generate_mipmaps(VkPhysicalDevice physicalDevice, VkCommandBuffer commandBuffer, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
void generate_mipmaps(
	VkPhysicalDevice physicalDevice,
	VkCommandBuffer commandBuffer,
	VkImage image,
	VkFormat imageFormat,
	int32_t texWidth,
	int32_t texHeight,
	uint32_t mipLevels,
	VkImageLayout currentLayout,
	VkImageLayout finalLayout);