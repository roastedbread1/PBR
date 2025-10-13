#include <vk_app.h>
#include <camera.h>
#include <line_canvas.h>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <vector>
#include <stb/stb_image_resize2.h>

const glm::vec3 kInitialCameraPos = glm::vec3(0.0f, 1.0f, -1.5f);
const glm::vec3 kInitialCameraTarget = glm::vec3(0.0f, 0.5f, 0.0f);
const glm::vec3 kInitialCameraAngles = glm::vec3(-18.5f, 180.0f, 0.0f);

const char* cameraType = "FirstPerson";
const char* comboBoxItems[] = { "FirstPerson", "MoveTo" };
const char* currentComboBoxItem = cameraType;

struct VertexData {
    glm::vec3 pos;
    glm::vec3 n;
    glm::vec2 tc;
};

lvk::Holder<lvk::RenderPipelineHandle> pipeline;
lvk::Holder<lvk::RenderPipelineHandle> pipelineSkybox;
lvk::Holder<lvk::BufferHandle> bufferIndices;
lvk::Holder<lvk::BufferHandle> bufferVertices;
lvk::Holder<lvk::BufferHandle> bufferPerFrame;
lvk::Holder<lvk::TextureHandle> texture;
lvk::Holder<lvk::TextureHandle> cubemapTex;
std::vector<uint32_t> indices;

LinearGraph fpsGraph;
LinearGraph sinGraph;
LineCanvas2D canvas2d;
LineCanvas3D canvas3d;

void draw_frame(App* app, uint32_t width, uint32_t height, float aspectRatio, float deltaSeconds);

void reinitCamera(App* app)
{
    if (!strcmp(cameraType, "FirstPerson")) {
        init_camera_first_person(&app->camera, kInitialCameraPos, kInitialCameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else if (!strcmp(cameraType, "MoveTo")) {

        init_camera_move_to(&app->camera, kInitialCameraPos, kInitialCameraAngles);
    }
}

int main()
{
    App app{};
    AppConfig config = { .initialCameraPos = kInitialCameraPos, .initialCameraTarget = kInitialCameraTarget };
    init_app(&app, &config);

    init_line_canvas2D(&canvas2d);
    init_line_canvas3D(&canvas3d);
    init_graph(&fpsGraph, "##fpsGraph", 2048);
    init_graph(&sinGraph, "##sinGraph", 2048);


    lvk::IContext& ctx = *app.ctx;
    
        lvk::Holder<lvk::ShaderModuleHandle> vert = load_shader_module(app.ctx, "D:/codes/more codes/c++/PBR/src/main.vert");
        lvk::Holder<lvk::ShaderModuleHandle> frag = load_shader_module(app.ctx, "D:/codes/more codes/c++/PBR/src/main.frag");
        lvk::Holder<lvk::ShaderModuleHandle> vertSkybox = load_shader_module(app.ctx, "D:/codes/more codes/c++/PBR/src/skybox.vert");
        lvk::Holder<lvk::ShaderModuleHandle> fragSkybox = load_shader_module(app.ctx, "D:/codes/more codes/c++/PBR/src/skybox.frag");

        const lvk::VertexInput vdesc = {
            .attributes = { {.location = 0, .format = lvk::VertexFormat::Float3, .offset = offsetof(VertexData, pos) },
                               {.location = 1, .format = lvk::VertexFormat::Float3, .offset = offsetof(VertexData, n) },
                               {.location = 2, .format = lvk::VertexFormat::Float2, .offset = offsetof(VertexData, tc) }, },
            .inputBindings = { {.stride = sizeof(VertexData) } },
        };

        pipeline = ctx.createRenderPipeline({
            .vertexInput = vdesc,
            .smVert = vert,
            .smFrag = frag,
            .color = { {.format = ctx.getSwapchainFormat() } },
            .depthFormat = get_depth_format_app(&app),
            .cullMode = lvk::CullMode_Back,
            });

        pipelineSkybox = ctx.createRenderPipeline({
            .smVert = vertSkybox,
            .smFrag = fragSkybox,
            .color = { {.format = ctx.getSwapchainFormat() } },
            .depthFormat = get_depth_format_app(&app),
            });

        const aiScene* scene = aiImportFile("D:/codes/more codes/c++/PBR/rubber_duck/scene.gltf", aiProcess_Triangulate);
        if (!scene || !scene->HasMeshes()) {
            printf("Unable to load data/rubber_duck/scene.gltf\n");
            exit(255);
        }

        const aiMesh* mesh = scene->mMeshes[0];
        std::vector<VertexData> vertices;
        for (uint32_t i = 0; i != mesh->mNumVertices; i++) {
            const aiVector3D v = mesh->mVertices[i];
            const aiVector3D n = mesh->mNormals[i];
            const aiVector3D t = mesh->mTextureCoords[0][i];
            vertices.push_back({ .pos = glm::vec3(v.x, v.y, v.z), .n = glm::vec3(n.x, n.y, n.z), .tc = glm::vec2(t.x, t.y) });
        }
        for (uint32_t i = 0; i != mesh->mNumFaces; i++) {
            for (uint32_t j = 0; j != 3; j++)
                indices.push_back(mesh->mFaces[i].mIndices[j]);
        }
        aiReleaseImport(scene);

        bufferIndices = ctx.createBuffer({ .usage = lvk::BufferUsageBits_Index, .storage = lvk::StorageType_Device, .size = sizeof(uint32_t) * indices.size(), .data = indices.data(), .debugName = "Buffer: indices" });
        bufferVertices = ctx.createBuffer({ .usage = lvk::BufferUsageBits_Vertex, .storage = lvk::StorageType_Device, .size = sizeof(VertexData) * vertices.size(), .data = vertices.data(), .debugName = "Buffer: vertices" });

        struct PerFrameData { glm::mat4 model, view, proj; glm::vec4 cameraPos; uint32_t tex = 0, texCube = 0; };
        bufferPerFrame = ctx.createBuffer({ .usage = lvk::BufferUsageBits_Uniform, .storage = lvk::StorageType_Device, .size = sizeof(PerFrameData), .debugName = "Buffer: per-frame" });

        texture = load_texture(app.ctx, "D:/codes/more codes/c++/PBR/rubber_duck/textures/Duck_baseColor.png");

        
        {
            int w, h;
            const float* img = stbi_loadf("D:/codes/more codes/c++/PBR/data/empty_play_room_4k.hdr", &w, &h, nullptr, 4);
            Bitmap in;
            init_bitmap_from_data(&in,w, h, 4, eBitmapFormat_Float, img);
            Bitmap out = convert_equirectangular_map_to_vertial_cross(in);
            stbi_image_free((void*)img);

            //stbi_write_hdr(".cache/screenshot.hdr", out.w, out.h, out.comp, (const float*)out.data.data());

            Bitmap cubemap = convert_vertical_cross_to_cubemap_faces(out);

            cubemapTex = ctx.createTexture({
                .type = lvk::TextureType_Cube,
                .format = lvk::Format_RGBA_F32,
                .dimensions = {(uint32_t)cubemap.w, (uint32_t)cubemap.h},
                .usage = lvk::TextureUsageBits_Sampled,
                .data = cubemap.data.data(),
                .debugName = "data/piazza_bologni_1k.hdr",
                });
        }
    
    run_app(&app, (DrawFrameFunc)draw_frame);


}

void draw_frame(App* app, uint32_t width, uint32_t height, float aspectRatio, float deltaSeconds)
{
    if (app->camera.positioner.type == CAMERA_POSITIONER_MOVE_TO) {
        update_move_to(
            &app->camera.positioner.moveTo,
            deltaSeconds,
            app->mouseState.pos,
            ImGui::GetIO().WantCaptureMouse ? false : app->mouseState.pressedLeft);
    }

    const glm::mat4 p = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 1000.0f);
    const glm::mat4 m1 = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
    const glm::mat4 m2 = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));

    struct PerFrameData { glm::mat4 model, view, proj; glm::vec4 cameraPos; uint32_t tex = 0, texCube = 0; };
    const PerFrameData perFrameData = {
        .model = m2 * m1,
        .view = get_view_matrix_camera(&app->camera),
        .proj = p,
        .cameraPos = glm::vec4(get_position_camera(&app->camera), 1.0f),
        .tex = texture.index(),
        .texCube = cubemapTex.index(),
    };

    const lvk::RenderPass renderPass = {
        .color = { {.loadOp = lvk::LoadOp_Clear, .clearColor = { 1.0f, 1.0f, 1.0f, 1.0f } } },
        .depth = {.loadOp = lvk::LoadOp_Clear, .clearDepth = 1.0f }
    };
    const lvk::Framebuffer framebuffer = {
        .color = { {.texture = app->ctx->getCurrentSwapchainTexture() } },
        .depthStencil = {.texture = get_depth_texture(app) },
    };

    lvk::ICommandBuffer& buf = app->ctx->acquireCommandBuffer();
    buf.cmdUpdateBuffer(bufferPerFrame, perFrameData);

    buf.cmdBeginRendering(renderPass, framebuffer);
    {
        buf.cmdPushDebugGroupLabel("Skybox", 0xff0000ff);
        buf.cmdBindRenderPipeline(pipelineSkybox);
        buf.cmdPushConstants(app->ctx->gpuAddress(bufferPerFrame));
        buf.cmdDraw(36);
        buf.cmdPopDebugGroupLabel();

        buf.cmdPushDebugGroupLabel("Mesh", 0xff0000ff);
        buf.cmdBindVertexBuffer(0, bufferVertices);
        buf.cmdBindRenderPipeline(pipeline);
        buf.cmdBindDepthState({ .compareOp = lvk::CompareOp_Less, .isDepthWriteEnabled = true });
        buf.cmdBindIndexBuffer(bufferIndices, lvk::IndexFormat_UI32);
        buf.cmdDrawIndexed(indices.size());
        buf.cmdPopDebugGroupLabel();

        app->imgui->beginFrame(framebuffer);

        draw_memo_app(app);
        draw_fps(app);

        ImGui::Begin("Camera Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::BeginCombo("##combo", currentComboBoxItem))
        {
            for (int n = 0; n < IM_ARRAYSIZE(comboBoxItems); n++) {
                const bool isSelected = (currentComboBoxItem == comboBoxItems[n]);
                if (ImGui::Selectable(comboBoxItems[n], isSelected))
                    currentComboBoxItem = comboBoxItems[n];
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (currentComboBoxItem && strcmp(currentComboBoxItem, cameraType)) {
            cameraType = currentComboBoxItem;
            reinitCamera(app);
        }
        ImGui::End();

        render_graph(&sinGraph, 0, height * 0.7f, width, height * 0.2f, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        render_graph(&fpsGraph, 0, height * 0.8f, width, height * 0.2f);

        clear_line_canvas2D(&canvas2d);
        push_line_canvas2D(&canvas2d, { 100, 300 }, { 100, 400 }, glm::vec4(1, 0, 0, 1));
        push_line_canvas2D(&canvas2d, { 100, 400 }, { 200, 400 }, glm::vec4(0, 1, 0, 1));
        push_line_canvas2D(&canvas2d, { 200, 400 }, { 200, 300 }, glm::vec4(0, 0, 1, 1));
        push_line_canvas2D(&canvas2d, { 200, 300 }, { 100, 300 }, glm::vec4(1, 1, 0, 1));
        render_line_canvas2D(&canvas2d, "##plane");

        clear_line_canvas3D(&canvas3d);
        set_matrix_line_canvas3D(&canvas3d,perFrameData.proj* perFrameData.view);
        plane_line_canvas3D(&canvas3d, glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), 40, 40, 10.0f, 10.0f, glm::vec4(1, 0, 0, 1), glm::vec4(0, 1, 0, 1));
        box_line_canvas3D(&canvas3d, glm::mat4(1.0f), BoundingBox(glm::vec3(-2), glm::vec3(+2)), glm::vec4(1, 1, 0, 1));
        frustum_line_canvas3D(&canvas3d,
            glm::lookAt(glm::vec3(cos(glfwGetTime()), kInitialCameraPos.y, sin(glfwGetTime())), kInitialCameraTarget, glm::vec3(0.0f, 1.0f, 0.0f)),
            glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 30.0f), glm::vec4(1, 1, 1, 1));
        render_line_canvas3D(&canvas3d, *app->ctx.get(), framebuffer, buf);

        ImGui::End();

        app->imgui->endFrame(buf);
    }
    buf.cmdEndRendering();

    app->ctx->submit(buf, app->ctx->getCurrentSwapchainTexture());

    
    add_point_graph(&fpsGraph, (float)get_fps(&app->fpsCounter));
    add_point_graph(&sinGraph, sinf((float)glfwGetTime() * 20.0f));
}
