#include "vk_app.h"
//#include <utils_gltf.h>
#include <volk.h>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <imgui.h>
#include <ImGuizmo.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <implot.h>
extern std::unordered_map<uint32_t, std::string> debugGLSLSourceCode;


static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	App* app = (App*)glfwGetWindowUserPointer(window);
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		app->mouseState.pressedLeft = action == GLFW_PRESS;
	}
	
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	const ImGuiMouseButton_ imguiButton = (button == GLFW_MOUSE_BUTTON_LEFT) ? 
		ImGuiMouseButton_Left : (button == GLFW_MOUSE_BUTTON_RIGHT ? ImGuiMouseButton_Right : ImGuiMouseButton_Middle);

	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2((float)xpos, (float)ypos);
	io.MouseDown[imguiButton] = action == GLFW_PRESS;

	for (auto& cb : app->callbacksMouseButton)
	{
		cb(window, button, action, mods);
	}
}


static void glfw_scroll_callback(GLFWwindow* window, double dx, double dy)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseWheelH = (float)dx;
	io.MouseWheel = (float)dy;
}

static void glfw_cursor_pos_callback(GLFWwindow* window, double x, double y)
{
	App* app = (App*)glfwGetWindowUserPointer(window);
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	ImGui::GetIO().MousePos = ImVec2(x, y);
	if (width > 0 && height > 0)
	{
		app->mouseState.pos.x = static_cast<float>(x / width);
		app->mouseState.pos.y = 1.0f - static_cast<float>(y / height);
	}

}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	App* app = (App*)glfwGetWindowUserPointer(window);
	const bool pressed = action != GLFW_RELEASE;
	if (key == GLFW_KEY_ESCAPE && pressed) { glfwSetWindowShouldClose(window, GLFW_TRUE); }
	if (key == GLFW_KEY_W) { app->camera.positioner.firstPerson.movement.forward = pressed; }
	if (key == GLFW_KEY_S) { app->camera.positioner.firstPerson.movement.backward = pressed; }
	if (key == GLFW_KEY_A) { app->camera.positioner.firstPerson.movement.left = pressed; }
	if (key == GLFW_KEY_D) { app->camera.positioner.firstPerson.movement.right = pressed; }

	app->camera.positioner.firstPerson.movement.fastSpeed = (mods & GLFW_MOD_SHIFT) != 0;

	if (key == GLFW_KEY_SPACE)
	{
		look_at_first_person_(&app->camera.positioner.firstPerson, app->cfg.initialCameraPos,app->cfg.initialCameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
		set_speed_first_person(&app->camera.positioner.firstPerson, glm::vec3(0));
	}

	for (auto& cb : app->callbacksKey)
	{
		cb(window, key, scancode, action, mods);
	}
}

static PFN_vkVoidFunction ImGui_Vulkan_Loader(const char* function_name, void* user_data)
{
	
	VkInstance instance = (VkInstance)user_data;
	return vkGetInstanceProcAddr(instance, function_name);
}
void init_app(App* app, const AppConfig* cfg)
{
	if (cfg)
	{
		app->cfg = *cfg;
	}
	else
	{
		init_default_app(&app->cfg);
	}

	init_fps(&app->fpsCounter);

	init_camera_first_person(&app->camera, app->cfg.initialCameraPos, app->cfg.initialCameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

	

	int width = 1920;
	int height = 1080;

	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialize GLFW");
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	app->window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
	if (!app->window) {
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}
	app->pipelineSamples = 1;

	VK_CHECK(volkInitialize());
	create_instance(&app->vkInstance.instance);
	volkLoadInstance(app->vkInstance.instance);

	setup_debug_callbacks(app->vkInstance.instance, &app->vkInstance.messenger, &app->vkInstance.reportCallback);
	VK_CHECK(glfwCreateWindowSurface(app->vkInstance.instance, app->window, nullptr, &app->vkInstance.surface));
	if (!init_vulkan_render_device3(app->vkInstance, app->vkDev, width, height, VulkanContextFeatures{})) {
		throw std::runtime_error("Failed to initialize Vulkan render device");
	}
	
	if (!create_depth_resources(app->vkDev, width, height, app->depthTexture.image)) {
		throw std::runtime_error("Failed to create depth resources");
	}
	RenderPassCreateInfo rpInfo = {};
	render_pass_init(&rpInfo, false, false, eRenderPassBit_Last);

	SwapchainSupportDetails details = query_swapchain_support(app->vkDev.physicalDevice, app->vkInstance.surface);
	VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(details.formats);

	if (!create_color_and_depth_render_pass(app->vkDev, true, &app->gridRenderPass, rpInfo, surfaceFormat.format)) {
		throw std::runtime_error("Failed to create main render pass");
	}

	create_swapchain_framebuffers(app);
	create_sync_objects(app);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	app->implotCtx = ImPlot::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(app->window, false);

	if (!create_descriptor_pool(app->vkDev, 100, 100, 100, &app->imguiDescriptorPool)) {
		throw std::runtime_error("Failed to create ImGui descriptor pool");
	}

	// Store swapchain format for ImGui
	SwapchainSupportDetails detailsImgui = query_swapchain_support(app->vkDev.physicalDevice, app->vkInstance.surface);
	VkSurfaceFormatKHR surfaceFormatImgui = choose_swap_surface_format(detailsImgui.formats);
	app->swapchainFormat = surfaceFormatImgui.format;

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = app->vkInstance.instance;
	init_info.PhysicalDevice = app->vkDev.physicalDevice;
	init_info.Device = app->vkDev.device;
	init_info.QueueFamily = app->vkDev.graphicsFamily;
	init_info.Queue = app->vkDev.graphicsQueue;
	init_info.DescriptorPool = app->imguiDescriptorPool;
	init_info.MinImageCount = choose_swap_image_count(detailsImgui.capabilities);
	init_info.ImageCount = (uint32_t)app->vkDev.swapchainImages.size();
	init_info.CheckVkResultFn = [](VkResult err) { VK_CHECK(err); };

	// Dynamic rendering setup
	init_info.UseDynamicRendering = true;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &app->swapchainFormat;

	uint32_t api_version = volkGetInstanceVersion();
	if (api_version == 0) {
		api_version = VK_API_VERSION_1_1;
	}
	ImGui_ImplVulkan_LoadFunctions(api_version, ImGui_Vulkan_Loader, app->vkInstance.instance);
	ImGui_ImplVulkan_Init(&init_info);


	glfwSetWindowUserPointer(app->window, app);
	glfwSetMouseButtonCallback(app->window, glfw_mouse_button_callback);
	glfwSetScrollCallback(app->window, glfw_scroll_callback);
	glfwSetCursorPosCallback(app->window, glfw_cursor_pos_callback);
	glfwSetKeyCallback(app->window, glfw_key_callback);
}

void destroy_app(App* app)
{
	

	vkDeviceWaitIdle(app->vkDev.device);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext(app->implotCtx); 
	ImGui::DestroyContext();

	VkDevice device = app->vkDev.device;
	for (int i = 0; i < app->inFlightFences.size(); i++) {
        vkDestroyFence(device, app->inFlightFences[i], nullptr);
        vkDestroySemaphore(device, app->renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, app->imageAvailableSemaphores[i], nullptr);
    }
	for (VkFramebuffer fb : app->swapchainFramebuffers) {
		vkDestroyFramebuffer(device, fb, nullptr);
	}

	vkDestroyPipeline(device, app->gridPipeline, nullptr);
	vkDestroyPipelineLayout(device, app->gridPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, app->gridDescriptorSetLayout, nullptr);
	vkDestroyRenderPass(device, app->gridRenderPass, nullptr);

	vkDestroyShaderModule(device, app->gridVert.shaderModule, nullptr);
	vkDestroyShaderModule(device, app->gridFrag.shaderModule, nullptr);

	destroy_vulkan_image(device, app->depthTexture.image);


	vkDestroyDescriptorPool(device, app->imguiDescriptorPool, nullptr);
	destroy_vulkan_render_device(app->vkDev);
	destroy_vulkan_instance(app->vkInstance);



	glfwDestroyWindow(app->window);
	glfwTerminate();
}

void recreate_swapchain(App* app)
{
	VulkanRenderDevice& vkDev = app->vkDev;
	vkDeviceWaitIdle(vkDev.device);

	
	for (auto fb : app->swapchainFramebuffers) {
		vkDestroyFramebuffer(vkDev.device, fb, nullptr);
	}
	app->swapchainFramebuffers.clear();

	for (auto iv : app->vkDev.swapchainImageViews) {
		vkDestroyImageView(vkDev.device, iv, nullptr);
	}
	app->vkDev.swapchainImageViews.clear();
	app->vkDev.swapchainImages.clear();

	vkDestroySwapchainKHR(vkDev.device, app->vkDev.swapchain, nullptr);
	destroy_vulkan_image(vkDev.device, app->depthTexture.image);

	
	int width, height;
	glfwGetFramebufferSize(app->window, &width, &height);
	while (width == 0 || height == 0) {
	
		glfwGetFramebufferSize(app->window, &width, &height);
		glfwWaitEvents();
	}
	vkDev.framebufferWidth = (uint32_t)width;
	vkDev.framebufferHeight = (uint32_t)height;


	app->depthTexture.width = width;
	app->depthTexture.height = height;
	app->depthTexture.format = find_depth_format(vkDev.physicalDevice);


	
	VK_CHECK(create_swapchain(vkDev.device, vkDev.physicalDevice, app->vkInstance.surface, vkDev.graphicsFamily, width, height, &vkDev.swapchain));
	create_swapchain_images(vkDev.device, vkDev.swapchain, vkDev.swapchainImages, vkDev.swapchainImageViews);
	create_depth_resources(vkDev, width, height, app->depthTexture.image);

	
	create_swapchain_framebuffers(app);
}

void run_app(App* app, DrawFrameFunc drawFrame)
{
	

	double timeStamp = glfwGetTime();
	float deltaSeconds = 0.0f;

	while (!glfwWindowShouldClose(app->window))
	{
		tick_fps(&app->fpsCounter, deltaSeconds);
		const double newTimeStamp = glfwGetTime();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;
		glfwPollEvents();
		int width, height;

#if defined(__APPLE__)
		glfwGetWindowSize(app->window, &width, &height);
#else 
		glfwGetFramebufferSize(app->window, &width, &height);
#endif

		if (!width || !height) { continue; }

		const float ratio = width / (float)height;

		bool blockMouse = ImGui::GetIO().WantCaptureMouse || ImGuizmo::IsUsing() || ImGuizmo::IsOver();
		bool mouseActive = blockMouse ? false : app->mouseState.pressedLeft;

		if (app->camera.positioner.type == CAMERA_POSITIONER_FIRST_PERSON) {
			update_first_person(
				&app->camera.positioner.firstPerson,
				deltaSeconds,
				app->mouseState.pos,
				ImGui::GetIO().WantCaptureMouse ? false : app->mouseState.pressedLeft
			);
		}

		if (app->camera.positioner.type == CAMERA_POSITIONER_MOVE_TO) {
			update_move_to(
				&app->camera.positioner.moveTo,
				deltaSeconds,
				app->mouseState.pos,
				ImGui::GetIO().WantCaptureMouse ? false : app->mouseState.pressedLeft
			);
		}


		drawFrame(app, (uint32_t)width, (uint32_t)height, ratio, deltaSeconds);
	}

}

void init_default_app(AppConfig* cfg)
{
	cfg->initialCameraPos = glm::vec3(0.0f, 0.0f, -2.5f);
	cfg->initialCameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	cfg->showGLTFInspector = false;
}

void draw_grid_app(App* app, VkCommandBuffer buf, const glm::mat4 proj, const glm::vec3 origin, uint32_t numSamples, VkFormat colorFormat)
{
	draw_grid_with_cam_pos_app(
		app,
		buf,
		proj * get_view_matrix_camera(&app->camera),
		origin,
		get_position_camera(&app->camera),
		numSamples,
		colorFormat
	);
}

void draw_grid_with_cam_pos_app(App* app, VkCommandBuffer buf, const glm::mat4& mvp, const glm::vec3& origin, const glm::vec3& camPos, uint32_t numSamples, VkFormat colorFormat)
{
	// 1. Create Pipeline if it doesn't exist. 
	// If samples change, we ignore it for now to prevent the crash.
	if (app->gridPipeline == VK_NULL_HANDLE) {
		app->pipelineSamples = numSamples;

		// Create Layout if missing
		if (app->gridDescriptorSetLayout == VK_NULL_HANDLE) {
			VkDescriptorSetLayoutCreateInfo dslInfo = {
			   .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			   .bindingCount = 0,
			   .pBindings = nullptr
			};
			VK_CHECK(vkCreateDescriptorSetLayout(app->vkDev.device, &dslInfo, nullptr, &app->gridDescriptorSetLayout));
		}

		// Create Pipeline Layout
		if (app->gridPipelineLayout == VK_NULL_HANDLE) {
			const struct PushConstants {
				glm::mat4 mvp;
				glm::vec4 camPos;
				glm::vec4 origin;
			};
			VkPushConstantRange pcRange = {
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = sizeof(PushConstants)
			};
			VkPipelineLayoutCreateInfo plInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				.setLayoutCount = 1,
				.pSetLayouts = &app->gridDescriptorSetLayout,
				.pushConstantRangeCount = 1,
				.pPushConstantRanges = &pcRange
			};
			VK_CHECK(vkCreatePipelineLayout(app->vkDev.device, &plInfo, nullptr, &app->gridPipelineLayout));
		}

		const std::vector<const char*> shaderFiles = {
			"D:/codes/more codes/c++/PBR/src/Grid.vert",
			"D:/codes/more codes/c++/PBR/src/Grid.frag"
		};

		VkFormat depthFormat = find_depth_format(app->vkDev.physicalDevice);

		if (!create_graphics_pipeline(
			app->vkDev,
			app->gridPipelineLayout,
			shaderFiles,
			&app->gridPipeline,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			colorFormat,
			depthFormat,
			true, false, true, true, // DepthTest=True, DepthWrite=False
			static_cast<VkSampleCountFlagBits>(app->pipelineSamples),
			-1, -1, 0
		)) {
			throw std::runtime_error("Failed to create grid pipeline");
		}
	}

	// 2. Record Commands
	const struct PushConstants {
		glm::mat4 mvp;
		glm::vec4 camPos;
		glm::vec4 origin;
	} pc = {
		.mvp = mvp,
		.camPos = glm::vec4(camPos, 1.0f),
		.origin = glm::vec4(origin, 1.0f),
	};

	vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gridPipeline);

	vkCmdPushConstants(
		buf,
		app->gridPipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PushConstants),
		&pc
	);

	vkCmdDraw(buf, 6, 1, 0, 0);
}
void draw_fps(App* app)
{
	if (const ImGuiViewport* v = ImGui::GetMainViewport()) {
		ImGui::SetNextWindowPos({ v->WorkPos.x + v->WorkSize.x - 15.0f, v->WorkPos.y + 15.0f }, ImGuiCond_Always, { 1.0f, 0.0f });
	}
	ImGui::SetNextWindowBgAlpha(0.30f);
	ImGui::SetNextWindowSize(ImVec2(ImGui::CalcTextSize("FPS : _______").x, 0));
	if (ImGui::Begin(
		"##FPS", nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
		ImGui::Text("FPS : %i", (int)get_fps(&app->fpsCounter));
		ImGui::Text("Ms  : %.1f", get_fps(&app->fpsCounter) > 0 ? 1000.0 / get_fps(&app->fpsCounter) : 0);
	}
	ImGui::End();
}

VkFormat get_depth_format_app(const App* app)
{
	return find_depth_format(app->vkDev.physicalDevice);
}

VulkanImage get_depth_texture(const App* app)
{
	return app->depthTexture.image;
}

void add_mouse_button_callback_app(App* app, GLFWmousebuttonfun cb)
{
	app->callbacksMouseButton.emplace_back(cb);
}

void add_key_callback_app(App* app, GLFWkeyfun cb)
{
	app->callbacksKey.emplace_back(cb);
}

void draw_memo_app(App* app)
{
	(void)app;
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::Begin(
		"Keyboard hints:", nullptr,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoCollapse);
	ImGui::Text("W/S/A/D - camera movement");
	ImGui::Text("1/2 - camera up/down");
	ImGui::Text("Shift - fast movement");
	ImGui::Text("Space - reset view");
	ImGui::End();
	
}

void create_swapchain_framebuffers(App* app)
{
	VulkanRenderDevice& vkDev = app->vkDev;

	for (VkFramebuffer fb : app->swapchainFramebuffers) {
		vkDestroyFramebuffer(vkDev.device, fb, nullptr);
	}

	app->swapchainFramebuffers.resize(vkDev.swapchainImageViews.size());
	for (size_t i = 0; i < vkDev.swapchainImageViews.size(); i++) {

		
		
		if (!create_color_and_depth_framebuffers(
			vkDev,
			vkDev.framebufferWidth,  
			vkDev.framebufferHeight, 
			app->gridRenderPass,     
			vkDev.swapchainImageViews[i], 
			app->depthTexture.image.imageView,  
			&app->swapchainFramebuffers[i]))
		{
			throw std::runtime_error("Failed to create swapchain framebuffer!");
		}
	}
}

void create_sync_objects(App* app)
{
	VulkanRenderDevice& vkDev = app->vkDev;


	//1.4 catches semaphore reuse, so changing this so each swapchain image has its own semaphore
	//const int MAX_FRAMES_IN_FLIGHT = 2;

	uint32_t imageCount = app->vkDev.swapchainImages.size();

	app->imageAvailableSemaphores.resize(imageCount);
	app->renderFinishedSemaphores.resize(imageCount);
	app->inFlightFences.resize(imageCount);
	app->currentFrame = 0;

	VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT 
	};

	for (int i = 0; i < imageCount; i++) {
		VK_CHECK(vkCreateSemaphore(vkDev.device, &semaphoreInfo, nullptr, &app->imageAvailableSemaphores[i]));
		VK_CHECK(vkCreateSemaphore(vkDev.device, &semaphoreInfo, nullptr, &app->renderFinishedSemaphores[i]));
		VK_CHECK(vkCreateFence(vkDev.device, &fenceInfo, nullptr, &app->inFlightFences[i]));
	}
}

void draw_GTF_inspector_app(App* app, GLTFIntrospective& intro)
{
	if (!app->cfg.showGLTFInspector)
	{
		return;
	}

	//ImGui::SetNextWindowPos(ImVec2(10, 300));

	draw_GTF_inspector_animations_app(app, intro);
	draw_GTF_inspector_materials(app, intro);
	draw_GTF_inspector_cameras(app, intro);
}

void draw_GTF_inspector_animations_app(App* app, GLTFIntrospective& intro)
{
	(void)app;
	if (!intro.showAnimations)
		return;

	if (ImGui::Begin("Animations", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse)) {
		for (uint32_t a = 0; a < intro.animations.size(); ++a) {
			auto it = std::find(intro.activeAnim.begin(), intro.activeAnim.end(), a);
			bool oState = it != intro.activeAnim.end();
			bool state = oState;
			ImGui::Checkbox(intro.animations[a].c_str(), &state);

			if (state) {
				if (!oState) {
					uint32_t freeSlot = intro.activeAnim.size() - 1;
					if (auto nf = std::find(intro.activeAnim.begin(), intro.activeAnim.end(), ~0u); nf != intro.activeAnim.end()) {
						freeSlot = std::distance(intro.activeAnim.begin(), nf);
					}
					intro.activeAnim[freeSlot] = a;
				}
			}
			else {
				if (it != intro.activeAnim.end()) {
					*it = ~0u;
				}
			}
		}
	}

	if (intro.showAnimationBlend) {
		ImGui::SliderFloat("Blend", &intro.blend, 0, 1.0f);
	}

	ImGui::End();
}

//void draw_GTF_inspector_materials(App* app, GLTFIntrospective& intro)
//{
//	(void)app;
//
//	if (!intro.showMaterials || intro.materials.empty()) { return; }
//
//	if (ImGui::Begin("Materials", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
//		| ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse))
//	{
//		for (uint32_t m = 0; m < intro.materials.size(); ++m) {
//			GLTFMaterialIntro& mat = intro.materials[m];
//			const uint32_t& currentMask = intro.materials[m].currentMaterialMask;
//
//			auto setMaterialMask = [&m = intro.materials[m]](uint32_t flag, bool active) {
//				m.modified = true;
//				if (active) {
//					m.currentMaterialMask |= flag;
//				}
//				else {
//					m.currentMaterialMask &= ~flag;
//				}
//				};
//
//			const bool isUnlit = (currentMask & MaterialType_Unlit) == MaterialType_Unlit;
//			bool state = false;
//
//			ImGui::Text("%s", mat.name.c_str());
//			ImGui::PushID(m);
//			state = isUnlit;
//
//			if (ImGui::RadioButton("Unlit", state)) {
//				mat.currentMaterialMask = 0;
//				setMaterialMask(MaterialType_Unlit, true);
//			}
//
//			state = (currentMask & MaterialType_MetallicRoughness) == MaterialType_MetallicRoughness;
//			if ((mat.materialMask & MaterialType_MetallicRoughness) == MaterialType_MetallicRoughness) {
//				if (ImGui::RadioButton("MetallicRoughness", state)) {
//					setMaterialMask(MaterialType_Unlit, false);
//					setMaterialMask(MaterialType_SpecularGlossiness, false);
//					setMaterialMask(MaterialType_MetallicRoughness, true);
//				}
//			}
//
//			state = (currentMask & MaterialType_SpecularGlossiness) == MaterialType_SpecularGlossiness;
//			if ((mat.materialMask & MaterialType_SpecularGlossiness) == MaterialType_SpecularGlossiness) {
//				if (ImGui::RadioButton("SpecularGlossiness", state)) {
//					setMaterialMask(MaterialType_Unlit, false);
//					setMaterialMask(MaterialType_SpecularGlossiness, true);
//					setMaterialMask(MaterialType_MetallicRoughness, false);
//				}
//			}
//
//			state = (currentMask & MaterialType_Sheen) == MaterialType_Sheen;
//			if ((mat.materialMask & MaterialType_Sheen) == MaterialType_Sheen) {
//				ImGui::BeginDisabled(isUnlit);
//				if (ImGui::Checkbox("Sheen", &state)) {
//					setMaterialMask(MaterialType_Sheen, state);
//				}
//				ImGui::EndDisabled();
//			}
//
//			state = (mat.currentMaterialMask & MaterialType_ClearCoat) == MaterialType_ClearCoat;
//			if ((mat.materialMask & MaterialType_ClearCoat) == MaterialType_ClearCoat) {
//				ImGui::BeginDisabled(isUnlit);
//				if (ImGui::Checkbox("ClearCoat", &state)) {
//					setMaterialMask(MaterialType_ClearCoat, state);
//				}
//				ImGui::EndDisabled();
//			}
//
//			state = (mat.currentMaterialMask & MaterialType_Specular) == MaterialType_Specular;
//			if ((mat.materialMask & MaterialType_Specular) == MaterialType_Specular) {
//				ImGui::BeginDisabled(isUnlit);
//				if (ImGui::Checkbox("Specular", &state)) {
//					setMaterialMask(MaterialType_Specular, state);
//				}
//				ImGui::EndDisabled();
//			}
//
//			state = (mat.currentMaterialMask & MaterialType_Transmission) == MaterialType_Transmission;
//			if ((mat.materialMask & MaterialType_Transmission) == MaterialType_Transmission) {
//				ImGui::BeginDisabled(isUnlit);
//				if (ImGui::Checkbox("Transmission", &state)) {
//					if (!state) {
//						setMaterialMask(MaterialType_Volume, false);
//					}
//					setMaterialMask(MaterialType_Transmission, state);
//				}
//				ImGui::EndDisabled();
//			}
//
//			state = (mat.currentMaterialMask & MaterialType_Volume) == MaterialType_Volume;
//			if ((mat.materialMask & MaterialType_Volume) == MaterialType_Volume) {
//				ImGui::BeginDisabled(isUnlit);
//				if (ImGui::Checkbox("Volume", &state)) {
//					setMaterialMask(MaterialType_Volume, state);
//					if (state) {
//						setMaterialMask(MaterialType_Transmission, true);
//					}
//				}
//				ImGui::EndDisabled();
//			}
//
//			ImGui::PopID();
//		}
//	}
//
//
//	ImGui::End();
//}

void draw_GTF_inspector_materials(App* app, GLTFIntrospective& intro)
{
	(void)app;
	if (!intro.showMaterials || intro.materials.empty()) return;

	
	ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Materials", nullptr, ImGuiWindowFlags_None))
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		
		ImGui::BeginChild("MaterialsList", ImVec2(0, 0), true);

		for (uint32_t m = 0; m < intro.materials.size(); ++m)
		{
			GLTFMaterialIntro& mat = intro.materials[m];
			const uint32_t& currentMask = mat.currentMaterialMask;
			const bool isUnlit = (currentMask & MaterialType_Unlit) == MaterialType_Unlit;

			auto setMaterialMask = [&mat](uint32_t flag, bool active) {
				mat.modified = true;
				if (active) {
					mat.currentMaterialMask |= flag;
				}
				else {
					mat.currentMaterialMask &= ~flag;
				}
				};

			ImGui::PushID(m);

			bool materialOpen = ImGui::CollapsingHeader(mat.name.empty() ?
				("Material " + std::to_string(m)).c_str() : mat.name.c_str());

			if (materialOpen)
			{
				ImGui::Indent();

				if (ImGui::RadioButton("Unlit", isUnlit)) {
					mat.currentMaterialMask = MaterialType_Unlit;
					mat.modified = true;
				}

				ImGui::SameLine();

				if ((mat.materialMask & MaterialType_MetallicRoughness) == MaterialType_MetallicRoughness) {
					bool state = (currentMask & MaterialType_MetallicRoughness) == MaterialType_MetallicRoughness;
					if (ImGui::RadioButton("MetallicRoughness", state)) {
						mat.currentMaterialMask = MaterialType_MetallicRoughness;
						mat.modified = true;
					}
					ImGui::SameLine();
				}

				if ((mat.materialMask & MaterialType_SpecularGlossiness) == MaterialType_SpecularGlossiness) {
					bool state = (currentMask & MaterialType_SpecularGlossiness) == MaterialType_SpecularGlossiness;
					if (ImGui::RadioButton("SpecularGlossiness", state)) {
						mat.currentMaterialMask = MaterialType_SpecularGlossiness;
						mat.modified = true;
					}
				}

				if (ImGui::BeginTable("MaterialFeatures", 2, ImGuiTableFlags_SizingFixedFit))
				{
					ImGui::TableNextColumn();

					if ((mat.materialMask & MaterialType_Sheen) == MaterialType_Sheen) {
						bool state = (currentMask & MaterialType_Sheen) == MaterialType_Sheen;
						ImGui::BeginDisabled(isUnlit);
						if (ImGui::Checkbox("Sheen", &state)) {
							setMaterialMask(MaterialType_Sheen, state);
						}
						ImGui::EndDisabled();
					}

					if ((mat.materialMask & MaterialType_ClearCoat) == MaterialType_ClearCoat) {
						bool state = (currentMask & MaterialType_ClearCoat) == MaterialType_ClearCoat;
						ImGui::BeginDisabled(isUnlit);
						if (ImGui::Checkbox("Clear Coat", &state)) {
							setMaterialMask(MaterialType_ClearCoat, state);
						}
						ImGui::EndDisabled();
					}

					ImGui::TableNextColumn();

					if ((mat.materialMask & MaterialType_Specular) == MaterialType_Specular) {
						bool state = (currentMask & MaterialType_Specular) == MaterialType_Specular;
						ImGui::BeginDisabled(isUnlit);
						if (ImGui::Checkbox("Specular", &state)) {
							setMaterialMask(MaterialType_Specular, state);
						}
						ImGui::EndDisabled();
					}

					if ((mat.materialMask & MaterialType_Transmission) == MaterialType_Transmission) {
						bool state = (currentMask & MaterialType_Transmission) == MaterialType_Transmission;
						ImGui::BeginDisabled(isUnlit);
						if (ImGui::Checkbox("Transmission", &state)) {
							if (!state) {
								setMaterialMask(MaterialType_Volume, false);
							}
							setMaterialMask(MaterialType_Transmission, state);
						}
						ImGui::EndDisabled();
					}

					if ((mat.materialMask & MaterialType_Volume) == MaterialType_Volume) {
						bool state = (currentMask & MaterialType_Volume) == MaterialType_Volume;
						ImGui::BeginDisabled(isUnlit);
						ImGui::Indent();
						if (ImGui::Checkbox("Volume", &state)) {
							setMaterialMask(MaterialType_Volume, state);
							if (state) {
								setMaterialMask(MaterialType_Transmission, true);
							}
						}
						ImGui::Unindent();
						ImGui::EndDisabled();
					}

					ImGui::EndTable();
				}

				ImGui::Spacing();
				ImGui::TextDisabled("Mask: 0x%08X", currentMask);

				ImGui::Unindent();
			}

			ImGui::PopID();

			if (m < intro.materials.size() - 1) {
				ImGui::Separator();
			}
		}

		ImGui::EndChild();  
	}
	ImGui::End();
}

void draw_GTF_inspector_cameras(App* app, GLTFIntrospective& intro)
{

	(void)app;
	if (!intro.showCameras)
		return;

	ImGui::Begin("Cameras:", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoCollapse);
	std::string current_item = intro.activeCamera != ~0u ? intro.cameras[intro.activeCamera] : "";
	if (ImGui::BeginCombo("##combo", current_item.c_str())) {
		for (uint32_t n = 0; n < intro.cameras.size(); n++) {
			bool is_selected = (current_item == intro.cameras[n]);
			if (ImGui::Selectable(intro.cameras[n].c_str(), is_selected)) {
				intro.activeCamera = n;
				current_item = intro.cameras[n];
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::End();
}
