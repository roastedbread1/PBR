#pragma once
#include <vk_app.h>
#include <vector>

struct BoundingBox;

typedef struct LineData2D
{
	glm::vec2 p1, p2;
	glm::vec4 color;
} LineData2D;

typedef struct LineCanvas2D
{
	std::vector<LineData2D> lines;
}LineCanvas2D;

void init_line_canvas2D(LineCanvas2D* canvas);
void clear_line_canvas2D(LineCanvas2D* canvas);
void push_line_canvas2D(LineCanvas2D* canvas, const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& c);
void render_line_canvas2D(const LineCanvas2D* canvas, const char* nameImGuiWindow);


typedef struct LineData3D
{
	glm::vec4 pos;
	glm::vec4 color;
}LineData3D;

typedef struct LineCanvas3D
{
	glm::mat4 mvp;
	std::vector<LineData3D> lines;

	ShaderModule vert;
	ShaderModule frag;
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VulkanBuffer linesBuffer[3];

	uint32_t pipelineSamples;
	uint32_t currentFrame;
} LineCanvas3D;

void init_line_canvas3D(LineCanvas3D* canvas);
void destroy_line_canvas3D(LineCanvas3D* canvas, VkDevice device);

void clear_line_canvas3D(LineCanvas3D* canvas);
void set_matrix_line_canvas3D(LineCanvas3D* canvas, const glm::mat4& mvp);
void render_line_canvas3D( LineCanvas3D* canvas, App* app, VkCommandBuffer buf, VkRenderPass renderPass, VkFormat colorFormat, VkFormat depthFormat, uint32_t numSamples = 1);
void push_line_canvas3D(LineCanvas3D* canvas, const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& c);
void plane_line_canvas3D(LineCanvas3D* canvas, const glm::vec3& orig, const glm::vec3& v1, const glm::vec3& v2, int n1, int n2, float s1, float s2, const glm::vec4& color, const glm::vec4& outlineColor);
void box_line_canvas3D(LineCanvas3D* canvas, const glm::mat4& m, const BoundingBox& box, const glm::vec4& color);
void box_line_canvas3D(LineCanvas3D* canvas, const glm::mat4& m, const glm::vec3& size, const glm::vec4& color);
void frustum_line_canvas3D(LineCanvas3D* canvas, const glm::mat4& camView, const glm::mat4& camProj, const glm::vec4& color);