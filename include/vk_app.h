#pragma once
#include <lvk/HelpersImGui.h>
#include <lvk/LVK.h>

#include <glfw/glfw3.h>
#include <implot/implot.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

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
	std::unique_ptr<lvk::IContext> ctx;
	lvk::Holder<lvk::TextureHandle> depthTexture;
	FramePerSecondCounter fpsCounter;
	lvk::ImGuiRenderer* imgui;
	ImPlotContext* implotCtx;

	AppConfig cfg;
	Camera camera;

	MouseState mouseState;

	lvk::Holder<lvk::ShaderModuleHandle> gridVert;
	lvk::Holder<lvk::ShaderModuleHandle> gridFrag;
	lvk::Holder<lvk::RenderPipelineHandle> gridPipeline;

	uint32_t pipelineSamples;

	std::vector<GLFWmousebuttonfun> callbacksMouseButton;
	std::vector<GLFWkeyfun> callbacksKey;


} App;


void init_app(App* app, const AppConfig* cfg);
void destroy_app(App* app);
void run_app(App* app, DrawFrameFunc drawFrame);
void init_default_app(AppConfig* cfg);

void draw_grid_app(App* app, lvk::ICommandBuffer& buf, const glm::mat4 proj, const glm::vec3 origin, uint32_t numSamples, lvk::Format colorFormat);
void draw_grid_with_cam_pos_app(App* app, lvk::ICommandBuffer& buf, const glm::mat4& mvp, const glm::vec3& origin, const glm::vec3& camPos, uint32_t numSamples, lvk::Format colorFormat);
void draw_fps(App* app);

void draw_GTF_inspector_app(App* app, GLTFIntrospective& intro);
void draw_GTF_inspector_animations_app(App* app, GLTFIntrospective& intro);
void draw_GTF_inspector_materials(App* app, GLTFIntrospective& intro);
void draw_GTF_inspector_cameras(App* app, GLTFIntrospective& intro);

lvk::Format get_depth_format_app(const App* app);
lvk::TextureHandle get_depth_texture(const App* app);

void add_mouse_button_callback_app(App* app, GLFWmousebuttonfun cb);
void add_key_callback_app(App* app, GLFWkeyfun cb);
void draw_memo_app(App* app);