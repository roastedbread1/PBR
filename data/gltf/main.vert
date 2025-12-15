//


#version 460
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_samplerless_texture_functions : require
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : require

    //  layout (set = 0, binding = 0) uniform texture2D kTextures2D[];
      // layout (set = 0, binding = 1) uniform sampler kSamplers[];
      // layout (set = 0, binding = 3) uniform sampler2D kSamplerYUV[];

      
      layout (set = 0, binding = 0) uniform sampler kSamplers[];     
      layout (set = 0, binding = 1) uniform sampler2D kSamplerYUV[];       
      layout (set = 0, binding = 3) uniform texture2D kTextures2D[];

      layout (set = 1, binding = 0) uniform texture3D kTextures3D[];
      layout (set = 2, binding = 0) uniform textureCube kTexturesCube[];
      layout (set = 3, binding = 0) uniform texture2D kTextures2DShadow[];
      layout (set = 3, binding = 1) uniform samplerShadow kSamplersShadow[];


      vec4 textureBindless2D(uint textureid, uint samplerid, vec2 uv) {
        return texture(nonuniformEXT(sampler2D(kTextures2D[textureid], kSamplers[samplerid])), uv);
      }
      vec4 textureBindless2DLod(uint textureid, uint samplerid, vec2 uv, float lod) {
        return textureLod(nonuniformEXT(sampler2D(kTextures2D[textureid], kSamplers[samplerid])), uv, lod);
      }
      float textureBindless2DShadow(uint textureid, uint samplerid, vec3 uvw) {
        return texture(nonuniformEXT(sampler2DShadow(kTextures2DShadow[textureid], kSamplersShadow[samplerid])), uvw);
      }
      ivec2 textureBindlessSize2D(uint textureid) {
        return textureSize(nonuniformEXT(kTextures2D[textureid]), 0);
      }
      vec4 textureBindlessCube(uint textureid, uint samplerid, vec3 uvw) {
        return texture(nonuniformEXT(samplerCube(kTexturesCube[textureid], kSamplers[samplerid])), uvw);
      }
      vec4 textureBindlessCubeLod(uint textureid, uint samplerid, vec3 uvw, float lod) {
        return textureLod(nonuniformEXT(samplerCube(kTexturesCube[textureid], kSamplers[samplerid])), uvw, lod);
      }
      int textureBindlessQueryLevels2D(uint textureid) {
        return textureQueryLevels(nonuniformEXT(kTextures2D[textureid]));
      }
      int textureBindlessQueryLevelsCube(uint textureid) {
        return textureQueryLevels(nonuniformEXT(kTexturesCube[textureid]));
      }

layout (location=0) out vec4 oUV0UV1;
layout (location=1) out vec3 oNormal;
layout (location=2) out vec3 oWorldPos;
layout (location=3) out vec4 oColor;
layout (location=4) out flat int oBaseInstance;

#include <D:/codes/more codes/c++/PBR/data/gltf/inputs.vert>

void main() {
  mat4 model = getModel();
  mat4 MVP = getViewProjection() * model;

  vec3 pos = getPosition();
  gl_Position = MVP * vec4(pos, 1.0);

  oUV0UV1 = vec4(getTexCoord(0), getTexCoord(1));
  oColor = getColor();

  mat3 normalMatrix = transpose( inverse(mat3(model)) );

  oNormal = normalMatrix  * getNormal();
  vec4 posClip = model * vec4(pos, 1.0);
  oWorldPos = posClip.xyz/posClip.w;

  oBaseInstance = gl_BaseInstance;
}
