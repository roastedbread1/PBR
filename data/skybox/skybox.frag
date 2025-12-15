
#version 460
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require



      layout (set = 0, binding = 0) uniform sampler kSamplers[];     
      layout (set = 0, binding = 1) uniform sampler2D kSamplerYUV[];       
      layout (set = 0, binding = 3) uniform texture2D kTextures2D[];

      layout (set = 1, binding = 0) uniform texture3D kTextures3D[];
      layout (set = 2, binding = 0) uniform textureCube kTexturesCube[];
      layout (set = 3, binding = 0) uniform texture2D kTextures2DShadow[];
      layout (set = 3, binding = 1) uniform samplerShadow kSamplersShadow[];

      vec4 textureBindlessCube(uint textureid, uint samplerid, vec3 uvw) {
        return texture(nonuniformEXT(samplerCube(kTexturesCube[textureid], kSamplers[samplerid])), uvw);
      }

layout(push_constant) uniform SkyboxPushConstants {
    mat4 view;
    mat4 proj;
    uint texCube;
    uint samplerIdx;
} pc;

layout (location=0) in vec3 dir;
layout (location=0) out vec4 out_FragColor;


void main() {
    vec3 color = textureLod(nonuniformEXT(samplerCube(kTexturesCube[pc.texCube], kSamplers[pc.samplerIdx])), normalize(dir), 0.0).rgb;
    out_FragColor = vec4(color, 1.0);
}