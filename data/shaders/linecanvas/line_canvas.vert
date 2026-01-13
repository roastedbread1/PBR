#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_buffer_reference : require

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
  Vertex v = vb.vertices[gl_VertexIndex];
  out_color = v.rgba;
  gl_Position = mvp * v.pos;
}