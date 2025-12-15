#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec4 in_color;
layout (location = 0) out vec4 out_color;

void main() {
  out_color = in_color;
}