
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

layout (location=0) out vec3 dir;

const vec3 pos[8] = vec3[8](
	vec3(-1.0,-1.0, 1.0),
	vec3( 1.0,-1.0, 1.0),
	vec3( 1.0, 1.0, 1.0),
	vec3(-1.0, 1.0, 1.0),
	vec3(-1.0,-1.0,-1.0),
	vec3( 1.0,-1.0,-1.0),
	vec3( 1.0, 1.0,-1.0),
	vec3(-1.0, 1.0,-1.0)
);

const int indices[36] = int[36](
	0, 1, 2, 2, 3, 0,
	1, 5, 6, 6, 2, 1,
	7, 6, 5, 5, 4, 7,
	4, 0, 3, 3, 7, 4,
	4, 5, 1, 1, 0, 4,
	3, 2, 6, 6, 7, 3
);

void main() {
	int idx = indices[gl_VertexIndex];
	gl_Position = pc.proj * mat4(mat3(pc.view)) * vec4(pos[idx], 1.0);
	dir = pos[idx].xyz;
}