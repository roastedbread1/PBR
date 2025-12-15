#pragma once
#define VK_NO_PROTOTYPES
#include <volk.h>
#include <utils_vulkan.h>
#include <utils_gltf.h>
#include <glfw/glfw3.h>
#include <implot.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <stb_image.h>
#include <stb_image_write.h>
#include <stdexcept>
#include <utils_fps.h>
#include <bitmap.h>
#include <camera.h>
#include <graph.h>
#include <utils.h>
#include <utils_cubemap.h>	
#include <utils_math.h>

#include <functional>

struct App;

typedef void (*DrawFrameFunc)(App* app, uint32_t width, uint32_t height, float aspectRatio, float deltaSeconds);

struct GLTFMaterialIntro
{
	std::string name;
	uint32_t materialMask;
	uint32_t currentMaterialMask;
	bool modified = false;
};

struct GLTFIntrospective
{
	std::vector<std::string> cameras;
	uint32_t activeCamera = ~0u;
	std::vector<std::string> animations;
	std::vector<uint32_t> activeAnim;

	std::vector<std::string> extensions;
	std::vector<uint32_t> activeExtensions;

	std::vector<GLTFMaterialIntro> materials;
	std::vector<bool> modifiedMaterial;

	float blend = 0.5f;

	bool showAnimations = false;
	bool showAnimationBlend= false;
	bool showCameras = false;
	bool showMaterials = false;
};

struct AppConfig
{
	glm::vec3 initialCameraPos;
	glm::vec3 initialCameraTarget;
	bool showGLTFInspector;
};

struct MouseState
{
	glm::vec2 pos = glm::vec2(0.0f);
	bool pressedLeft = false;
};

typedef struct App
{
	GLFWwindow* window;
	VulkanInstance vkInstance;
	VulkanRenderDevice vkDev;
	VulkanTexture depthTexture;
	FramePerSecondCounter fpsCounter;
	ImPlotContext* implotCtx;
	
	AppConfig cfg;
	Camera camera;

	ShaderModule gridVert;
	ShaderModule gridFrag;
	VkPipeline gridPipeline;


	VkPipelineLayout gridPipelineLayout;
	VkDescriptorSetLayout gridDescriptorSetLayout;
	VkRenderPass gridRenderPass;
	VkDescriptorPool imguiDescriptorPool;

	std::vector<VkFramebuffer> swapchainFramebuffers;

	//sync
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	std::vector<GLFWmousebuttonfun> callbacksMouseButton;
	std::vector<GLFWkeyfun> callbacksKey;


	uint32_t currentFrame;
	uint32_t pipelineSamples;
	MouseState mouseState;
	VkFormat swapchainFormat;

} App;


void init_app(App* app, const AppConfig* cfg);
void destroy_app(App* app);
void run_app(App* app, DrawFrameFunc drawFrame);
void init_default_app(AppConfig* cfg);

void draw_grid_app(App* app, VkCommandBuffer buf, const glm::mat4 proj, const glm::vec3 origin, uint32_t numSamples = 1, VkFormat colorFormat = VK_FORMAT_UNDEFINED);
void draw_grid_with_cam_pos_app(App* app, VkCommandBuffer buf, const glm::mat4& mvp, const glm::vec3& origin, const glm::vec3& camPos, uint32_t numSamples, VkFormat colorFormat);
void draw_fps(App* app);

void draw_GTF_inspector_app(App* app, GLTFIntrospective& intro);
void draw_GTF_inspector_animations_app(App* app, GLTFIntrospective& intro);
void draw_GTF_inspector_materials(App* app, GLTFIntrospective& intro);
void draw_GTF_inspector_cameras(App* app, GLTFIntrospective& intro);

VkFormat get_depth_format_app(const App* app);
VulkanImage get_depth_texture(const App* app);

void add_mouse_button_callback_app(App* app, GLFWmousebuttonfun cb);
void add_key_callback_app(App* app, GLFWkeyfun cb);
void draw_memo_app(App* app);


void create_swapchain_framebuffers(App* app);
void create_sync_objects(App* app);
void recreate_swapchain(App* app);