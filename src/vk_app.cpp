#include "vk_app.h"
//#include <utils_gltf.h>

#include <unordered_map>
#include <algorithm>
#include <vector>

extern std::unordered_map<uint32_t, std::string> debugGLSLSourceCode;

static void shaderModuleCallback(lvk::IContext*, lvk::ShaderModuleHandle handle, int line, int col, const char* debugName)
{
    const auto it = debugGLSLSourceCode.find(handle.index());
    if (it != debugGLSLSourceCode.end()) {
        lvk::logShaderSource(it->second.c_str());
    }
}


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

	minilog::initialize(nullptr, { .threadNames = false });

	int width = -95;
	int height = -90;

	app->window = lvk::initWindow("vulkan", width, height);
	app->pipelineSamples = 1;

		app->ctx = lvk::createVulkanContextWithSwapchain(
		app->window, width, height,
		{
			.enableValidation = true,
			.shaderModuleErrorCallback = &shaderModuleCallback,
		});

	
	
	app->depthTexture = app->ctx->createTexture(
		{
			.type = lvk::TextureType_2D,
			.format = lvk::Format_Z_F32,
			.dimensions = {(uint32_t)width, (uint32_t)height},
			.usage = lvk::TextureUsageBits_Attachment,
			.debugName = "Debug Buffer",
		}
		);

	app->imgui = new lvk::ImGuiRenderer(*app->ctx, "D:/codes/more codes/c++/PBR/OpenSans-Light.ttf", 30.0f);
	app->implotCtx = ImPlot::CreateContext();

	glfwSetWindowUserPointer(app->window, app);
	glfwSetMouseButtonCallback(app->window, glfw_mouse_button_callback);
	glfwSetScrollCallback(app->window, glfw_scroll_callback);
	glfwSetCursorPosCallback(app->window, glfw_cursor_pos_callback);
	glfwSetKeyCallback(app->window, glfw_key_callback);
}

void destroy_app(App* app)
{
	ImPlot::DestroyContext(app->implotCtx);

	app->gridPipeline = nullptr;
	app->gridVert = nullptr;
	app->gridFrag = nullptr;
	app->depthTexture = nullptr;

	delete app->imgui;
	app->imgui = nullptr;

	glfwDestroyWindow(app->window);


	glfwTerminate();
}

void run_app(App* app, DrawFrameFunc drawFrame)
{
	LVK_PROFILER_FUNCTION();

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

void draw_grid_app(App* app, lvk::ICommandBuffer& buf, const glm::mat4 proj, const glm::vec3 origin, uint32_t numSamples, lvk::Format colorFormat)
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

void draw_grid_with_cam_pos_app(App* app, lvk::ICommandBuffer& buf, const glm::mat4& mvp, const glm::vec3& origin, const glm::vec3& camPos, uint32_t numSamples, lvk::Format colorFormat)
{
	LVK_PROFILER_FUNCTION();

	if (app->gridPipeline.empty() || app->pipelineSamples != numSamples) {
		app->gridVert = load_shader_module(app->ctx, "D:/codes/more codes/c++/PBR/src/Grid.vert");
		app->gridFrag = load_shader_module(app->ctx, "D:/codes/more codes/c++/PBR/src/Grid.frag");

		app->pipelineSamples = numSamples;

		app->gridPipeline = app->ctx->createRenderPipeline({
			.smVert = app->gridVert,
			.smFrag = app->gridFrag,
			.color = { {
				.format = colorFormat != lvk::Format_Invalid ? colorFormat : app->ctx->getSwapchainFormat(),
				.blendEnabled = true,
				.srcRGBBlendFactor = lvk::BlendFactor_SrcAlpha,
				.dstRGBBlendFactor = lvk::BlendFactor_OneMinusSrcAlpha,
			} },
			.depthFormat = get_depth_format_app(app),
			.samplesCount = numSamples,
			.debugName = "Pipeline: drawGrid()",
			});
	}

	const struct {
		glm::mat4 mvp;
		glm::vec4 camPos;
		glm::vec4 origin;
	} pc = {
		.mvp = mvp,
		.camPos = glm::vec4(camPos, 1.0f),
		.origin = glm::vec4(origin, 1.0f),
	};

	buf.cmdPushDebugGroupLabel("Grid", 0xff0000ff);
	buf.cmdBindRenderPipeline(app->gridPipeline);
	buf.cmdBindDepthState({ .compareOp = lvk::CompareOp_Less, .isDepthWriteEnabled = false });
	buf.cmdPushConstants(pc);
	buf.cmdDraw(6);
	buf.cmdPopDebugGroupLabel();
}

void draw_fps(App* app)
{
	if (const ImGuiViewport* v = ImGui::GetMainViewport()) {
		ImGui::SetNextWindowPos({ v->WorkPos.x + v->WorkSize.x - 15.0f, v->WorkPos.y + 15.0f }, ImGuiCond_Always, { 1.0f, 0.0f });
	}
	ImGui::SetNextWindowBgAlpha(0.30f);
	ImGui::SetNextWindowSize(ImVec2(ImGui::CalcTextSize("FPS : _______").x, 0));
	if (ImGui::Begin("##FPS", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove)) {
		ImGui::Text("FPS : %i", (int)get_fps(&app->fpsCounter));
		ImGui::Text("Ms  : %.1f", get_fps(&app->fpsCounter) > 0 ? 1000.0 / get_fps(&app->fpsCounter) : 0);
	}
	ImGui::End();
}

lvk::Format get_depth_format_app(const App* app)
{
	return app->ctx->getFormat(app->depthTexture);
}

lvk::TextureHandle get_depth_texture(const App* app)
{
	return app->depthTexture;
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

void draw_GTF_inspector_app(App* app, GLTFIntrospective& intro)
{
	if (!app->cfg.showGLTFInspector)
	{
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(10, 300));

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

void draw_GTF_inspector_materials(App* app, GLTFIntrospective& intro)
{/*
	(void)app;
	LVK_PROFILER_FUNCTION();

	if (!intro.showMaterials || intro.materials.empty()) { return; }

	if (ImGui::Begin("Materials", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse))
	{
		for (uint32_t m = 0; m < intro.materials.size(); ++m) {
			GLTFMaterialIntro& mat = intro.materials[m];
			const uint32_t& currentMask = intro.materials[m].currentMaterialMask;

			auto setMaterialMask = [&m = intro.materials[m]](uint32_t flag, bool active) {
				m.modified = true;
				if (active) {
					m.currentMaterialMask |= flag;
				}
				else {
					m.currentMaterialMask &= ~flag;
				}
				};

			const bool isUnlit = (currentMask & MaterialType_Unlit) == MaterialType_Unlit;
			bool state = false;

			ImGui::Text("%s", mat.name.c_str());
			ImGui::PushID(m);
			state = isUnlit;

			if (ImGui::RadioButton("Unlit", state)) {
				mat.currentMaterialMask = 0;
				setMaterialMask(MaterialType_Unlit, true);
			}

			state = (currentMask & MaterialType_MetallicRoughness) == MaterialType_MetallicRoughness;
			if ((mat.materialMask & MaterialType_MetallicRoughness) == MaterialType_MetallicRoughness) {
				if (ImGui::RadioButton("MetallicRoughness", state)) {
					setMaterialMask(MaterialType_Unlit, false);
					setMaterialMask(MaterialType_SpecularGlossiness, false);
					setMaterialMask(MaterialType_MetallicRoughness, true);
				}
			}

			state = (currentMask & MaterialType_SpecularGlossiness) == MaterialType_SpecularGlossiness;
			if ((mat.materialMask & MaterialType_SpecularGlossiness) == MaterialType_SpecularGlossiness) {
				if (ImGui::RadioButton("SpecularGlossiness", state)) {
					setMaterialMask(MaterialType_Unlit, false);
					setMaterialMask(MaterialType_SpecularGlossiness, true);
					setMaterialMask(MaterialType_MetallicRoughness, false);
				}
			}

			state = (currentMask & MaterialType_Sheen) == MaterialType_Sheen;
			if ((mat.materialMask & MaterialType_Sheen) == MaterialType_Sheen) {
				ImGui::BeginDisabled(isUnlit);
				if (ImGui::Checkbox("Sheen", &state)) {
					setMaterialMask(MaterialType_Sheen, state);
				}
				ImGui::EndDisabled();
			}

			state = (mat.currentMaterialMask & MaterialType_ClearCoat) == MaterialType_ClearCoat;
			if ((mat.materialMask & MaterialType_ClearCoat) == MaterialType_ClearCoat) {
				ImGui::BeginDisabled(isUnlit);
				if (ImGui::Checkbox("ClearCoat", &state)) {
					setMaterialMask(MaterialType_ClearCoat, state);
				}
				ImGui::EndDisabled();
			}

			state = (mat.currentMaterialMask & MaterialType_Specular) == MaterialType_Specular;
			if ((mat.materialMask & MaterialType_Specular) == MaterialType_Specular) {
				ImGui::BeginDisabled(isUnlit);
				if (ImGui::Checkbox("Specular", &state)) {
					setMaterialMask(MaterialType_Specular, state);
				}
				ImGui::EndDisabled();
			}

			state = (mat.currentMaterialMask & MaterialType_Transmission) == MaterialType_Transmission;
			if ((mat.materialMask & MaterialType_Transmission) == MaterialType_Transmission) {
				ImGui::BeginDisabled(isUnlit);
				if (ImGui::Checkbox("Transmission", &state)) {
					if (!state) {
						setMaterialMask(MaterialType_Volume, false);
					}
					setMaterialMask(MaterialType_Transmission, state);
				}
				ImGui::EndDisabled();
			}

			state = (mat.currentMaterialMask & MaterialType_Volume) == MaterialType_Volume;
			if ((mat.materialMask & MaterialType_Volume) == MaterialType_Volume) {
				ImGui::BeginDisabled(isUnlit);
				if (ImGui::Checkbox("Volume", &state)) {
					setMaterialMask(MaterialType_Volume, state);
					if (state) {
						setMaterialMask(MaterialType_Transmission, true);
					}
				}
				ImGui::EndDisabled();
			}

			ImGui::PopID();
		}
	}

	ImGui::End();*/
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
