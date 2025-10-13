#include <line_canvas.h>

static const char* codeVS = R"(
layout (location = 0) out vec4 out_color;

struct Vertex {
  vec4 pos;
  vec4 rgba;
};

layout(std430, buffer_reference) readonly buffer VertexBuffer {
  Vertex vertices[];
};

layout(push_constant) uniform PushConstants {
  mat4 mvp;
  VertexBuffer vb;
};

void main() {
  out_color = vb.vertices[gl_VertexIndex].rgba;
  gl_Position = mvp * vb.vertices[gl_VertexIndex].pos;
})";

static const char* codeFS = R"(
layout (location = 0) in vec4 in_color;
layout (location = 0) out vec4 out_color;

void main() {
  out_color = in_color;
})";

void init_line_canvas2D(LineCanvas2D* canvas)
{
	canvas->lines.clear();
}

void clear_line_canvas2D(LineCanvas2D* canvas)
{
	canvas->lines.clear();
}

void push_line_canvas2D(LineCanvas2D* canvas, const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& c)
{
	canvas->lines.push_back({ .p1 = p1, .p2 = p2, .color = c });
}

void render_line_canvas2D(const LineCanvas2D* canvas, const char* nameImGuiWindow)
{

	LVK_PROFILER_FUNCTION();

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
	ImGui::Begin(
		nameImGuiWindow, nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

	ImDrawList* drawList = ImGui::GetBackgroundDrawList();

	for (const LineData2D l : canvas->lines)
	{
		drawList->AddLine(ImVec2(l.p1.x, l.p1.y), ImVec2(l.p2.x, l.p2.y), ImColor(l.color.r, l.color.g, l.color.b, l.color.a));
	}
	ImGui::End;
}

void init_line_canvas3D(LineCanvas3D* canvas)
{
	canvas->mvp = glm::mat4(1.0f);
	canvas->lines.clear();
	canvas->pipeline = nullptr;
	canvas->vert = nullptr;
	canvas->frag = nullptr;
	canvas->pipelineSamples = 1;
	canvas->currentFrame = 0;
	
	for (int i = 0; i < 3; i++)
	{
		canvas->linesBuffer[i] = nullptr;
		canvas->currentBufferSize[i] = 0;
	}

}

void destroy_line_canvas3D(LineCanvas3D* canvas)
{
	for (int i = 0; i < 3; i++)
	{
		canvas->linesBuffer[i] = nullptr;
	}

	canvas->pipeline = nullptr;
	canvas->vert = nullptr;
	canvas->frag = nullptr;
}

void clear_line_canvas3D(LineCanvas3D* canvas)
{
	canvas->lines.clear();
}

void set_matrix_line_canvas3D(LineCanvas3D* canvas, const glm::mat4& mvp)
{
	canvas->mvp = mvp;
}

void render_line_canvas3D(LineCanvas3D* canvas, lvk::IContext& ctx, const lvk::Framebuffer& desc, lvk::ICommandBuffer& buf, uint32_t numSamples)
{
	LVK_PROFILER_FUNCTION();

	if (canvas->lines.empty()) {
		return;
	}

	const uint32_t requiredSize = canvas->lines.size() * sizeof(LineData3D);

	if (canvas->currentBufferSize[canvas->currentFrame] < requiredSize) {
		canvas->linesBuffer[canvas->currentFrame] = ctx.createBuffer(
			{ .usage = lvk::BufferUsageBits_Storage, .storage = lvk::StorageType_HostVisible, .size = requiredSize, .data = canvas->lines.data() });
		canvas->currentBufferSize[canvas->currentFrame] = requiredSize;
	}
	else {
		ctx.upload(canvas->linesBuffer[canvas->currentFrame], canvas->lines.data(), requiredSize);
	}

	if (canvas->pipeline.empty() || canvas->pipelineSamples != numSamples) {
		canvas->pipelineSamples = numSamples;

		canvas->vert = ctx.createShaderModule({ codeVS, lvk::Stage_Vert, "Shader Module: LineCanvas3D (vert)" });
		canvas->frag = ctx.createShaderModule({ codeFS, lvk::Stage_Frag, "Shader Module: LineCanvas3D (frag)" });
		canvas->pipeline = ctx.createRenderPipeline(
			{
				.topology = lvk::Topology_Line,
				.smVert = canvas->vert,
				.smFrag = canvas->frag,
				.color = { {
					.format = ctx.getFormat(desc.color[0].texture),
					.blendEnabled = true,
					.srcRGBBlendFactor = lvk::BlendFactor_SrcAlpha,
					.dstRGBBlendFactor = lvk::BlendFactor_OneMinusSrcAlpha,
				} },
				.depthFormat = desc.depthStencil.texture ? ctx.getFormat(desc.depthStencil.texture) : lvk::Format_Invalid,
				.cullMode = lvk::CullMode_None,
				.samplesCount = numSamples,
			},
			nullptr);
	}

	struct {
		glm::mat4 mvp;
		uint64_t addr;
	} pc{
		.mvp = canvas->mvp,
		.addr = ctx.gpuAddress(canvas->linesBuffer[canvas->currentFrame]),
	};
	buf.cmdBindRenderPipeline(canvas->pipeline);
	buf.cmdPushConstants(pc);
	buf.cmdDraw(canvas->lines.size());

	canvas->currentFrame = (canvas->currentFrame + 1) % LVK_ARRAY_NUM_ELEMENTS(canvas->linesBuffer);
}

void push_line_canvas3D(LineCanvas3D* canvas, const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& c)
{
	canvas->lines.push_back({ .pos = glm::vec4(p1, 1.0f), .color = c });
	canvas->lines.push_back({ .pos = glm::vec4(p2, 1.0f), .color = c });
}

void plane_line_canvas3D(LineCanvas3D* canvas, const glm::vec3& orig, const glm::vec3& v1, const glm::vec3& v2, int n1, int n2, float s1, float s2, const glm::vec4& color, const glm::vec4& outlineColor)
{
	push_line_canvas3D(canvas, orig - s1 / 2.0f * v1 - s2 / 2.0f * v2, orig - s1 / 2.0f * v1 + s2 / 2.0f * v2, outlineColor);
	push_line_canvas3D(canvas, orig + s1 / 2.0f * v1 - s2 / 2.0f * v2, orig + s1 / 2.0f * v1 + s2 / 2.0f * v2, outlineColor);
	push_line_canvas3D(canvas, orig - s1 / 2.0f * v1 + s2 / 2.0f * v2, orig + s1 / 2.0f * v1 + s2 / 2.0f * v2, outlineColor);
	push_line_canvas3D(canvas, orig - s1 / 2.0f * v1 - s2 / 2.0f * v2, orig + s1 / 2.0f * v1 - s2 / 2.0f * v2, outlineColor);

	for (int i = 1; i < n1; i++) {
		float t = ((float)i - (float)n1 / 2.0f) * s1 / (float)n1;
		const glm::vec3 o1 = orig + t * v1;
		push_line_canvas3D(canvas, o1 - s2 / 2.0f * v2, o1 + s2 / 2.0f * v2, color);
	}
	for (int i = 1; i < n2; i++) {
		const float t = ((float)i - (float)n2 / 2.0f) * s2 / (float)n2;
		const glm::vec3 o2 = orig + t * v2;
		push_line_canvas3D(canvas, o2 - s1 / 2.0f * v1, o2 + s1 / 2.0f * v1, color);
	}
}

void box_line_canvas3D(LineCanvas3D* canvas, const glm::mat4& m, const BoundingBox& box, const glm::vec4& color)
{
	box_line_canvas3D(canvas, m * glm::translate(glm::mat4(1.f), .5f * (box.min + box.max)), 0.5f * glm::vec3(box.max - box.min), color);
}

void box_line_canvas3D(LineCanvas3D* canvas, const glm::mat4& m, const glm::vec3& size, const glm::vec4& color)
{
	glm::vec3 pts[8] = {
   glm::vec3(+size.x, +size.y, +size.z), glm::vec3(+size.x, +size.y, -size.z), glm::vec3(+size.x, -size.y, +size.z), glm::vec3(+size.x, -size.y, -size.z),
   glm::vec3(-size.x, +size.y, +size.z), glm::vec3(-size.x, +size.y, -size.z), glm::vec3(-size.x, -size.y, +size.z), glm::vec3(-size.x, -size.y, -size.z),
	};

	for (auto& p : pts)
		p = glm::vec3(m * glm::vec4(p, 1.f));

	push_line_canvas3D(canvas, pts[0], pts[1], color); push_line_canvas3D(canvas, pts[2], pts[3], color);
	push_line_canvas3D(canvas, pts[4], pts[5], color); push_line_canvas3D(canvas, pts[6], pts[7], color);
	push_line_canvas3D(canvas, pts[0], pts[2], color); push_line_canvas3D(canvas, pts[1], pts[3], color);
	push_line_canvas3D(canvas, pts[4], pts[6], color); push_line_canvas3D(canvas, pts[5], pts[7], color);
	push_line_canvas3D(canvas, pts[0], pts[4], color); push_line_canvas3D(canvas, pts[1], pts[5], color);
	push_line_canvas3D(canvas, pts[2], pts[6], color); push_line_canvas3D(canvas, pts[3], pts[7], color);
}

void frustum_line_canvas3D(LineCanvas3D* canvas, const glm::mat4& camView, const glm::mat4& camProj, const glm::vec4& color)
{
	const glm::vec3 corners[] = { glm::vec3(-1, -1, -1), glm::vec3(+1, -1, -1), glm::vec3(+1, +1, -1), glm::vec3(-1, +1, -1),
						 glm::vec3(-1, -1, +1), glm::vec3(+1, -1, +1), glm::vec3(+1, +1, +1), glm::vec3(-1, +1, +1) };

	glm::vec3 pp[8];

	for (int i = 0; i < 8; i++) {
		glm::vec4 q = glm::inverse(camView) * glm::inverse(camProj) * glm::vec4(corners[i], 1.0f);
		pp[i] = glm::vec3(q.x / q.w, q.y / q.w, q.z / q.w);
	}
	push_line_canvas3D(canvas,pp[0], pp[4], color);
	push_line_canvas3D(canvas, pp[1], pp[5], color);
	push_line_canvas3D(canvas, pp[2], pp[6], color);
	push_line_canvas3D(canvas, pp[3], pp[7], color);
	// near
	push_line_canvas3D(canvas, pp[0], pp[1], color);
	push_line_canvas3D(canvas, pp[1], pp[2], color);
	push_line_canvas3D(canvas, pp[2], pp[3], color);
	push_line_canvas3D(canvas, pp[3], pp[0], color);
	// x
	push_line_canvas3D(canvas, pp[0], pp[2], color);
	push_line_canvas3D(canvas, pp[1], pp[3], color);
	// far
	push_line_canvas3D(canvas, pp[4], pp[5], color);
	push_line_canvas3D(canvas, pp[5], pp[6], color);
	push_line_canvas3D(canvas, pp[6], pp[7], color);
	push_line_canvas3D(canvas, pp[7], pp[4], color);
	// x
	push_line_canvas3D(canvas, pp[4], pp[6], color);
	push_line_canvas3D(canvas, pp[5], pp[7], color);

	const glm::vec4 gridColor = color * 0.7f;
	const int gridLines = 100;

	// bottom
	{
		glm::vec3 p1 = pp[0];
		glm::vec3 p2 = pp[1];
		const glm::vec3 s1 = (pp[4] - pp[0]) / float(gridLines);
		const glm::vec3 s2 = (pp[5] - pp[1]) / float(gridLines);
		for (int i = 0; i != gridLines; i++, p1 += s1, p2 += s2)
			push_line_canvas3D(canvas, p1, p2, gridColor);
	}
	// top
	{
		glm::vec3 p1 = pp[2];
		glm::vec3 p2 = pp[3];
		const glm::vec3 s1 = (pp[6] - pp[2]) / float(gridLines);
		const glm::vec3 s2 = (pp[7] - pp[3]) / float(gridLines);
		for (int i = 0; i != gridLines; i++, p1 += s1, p2 += s2)
			push_line_canvas3D(canvas, p1, p2, gridColor);
	}
	// left
	{
		glm::vec3 p1 = pp[0];
		glm::vec3 p2 = pp[3];
		const glm::vec3 s1 = (pp[4] - pp[0]) / float(gridLines);
		const glm::vec3 s2 = (pp[7] - pp[3]) / float(gridLines);
		for (int i = 0; i != gridLines; i++, p1 += s1, p2 += s2)
			push_line_canvas3D(canvas, p1, p2, gridColor);
	}
	// right
	{
		glm::vec3 p1 = pp[1];
		glm::vec3 p2 = pp[2];
		const glm::vec3 s1 = (pp[5] - pp[1]) / float(gridLines);
		const glm::vec3 s2 = (pp[6] - pp[2]) / float(gridLines);
		for (int i = 0; i != gridLines; i++, p1 += s1, p2 += s2)
			push_line_canvas3D(canvas, p1, p2, gridColor);
	}
}









