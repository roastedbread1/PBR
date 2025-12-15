#pragma once
#include <volk.h>

#include <assimp/GltfMaterial.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <glm/gtx/norm.hpp>
#include <glm/ext.hpp>
#include <stb_image_resize2.h>
#include <unordered_map>
#include <memory>
#include <map>
#include <ktxvulkan.h>
#include <ktx.h>
#include <imgui.h>
#include <ImGuizmo.h>



#include <utils_vulkan.h>
typedef struct LineCanvas3D LineCanvas3D;
typedef struct GLTFIntrospective GLTFIntrospective;
struct App;

enum Distribution : uint32_t
{
	Distribution_Lambertian = 0,
	Distribution_GGX = 1,
	Distribution_Charlie =2,
};

enum MaterialType : uint32_t
{
	MaterialType_Invalid = 0,
	MaterialType_Unlit = 0x80,
	MaterialType_MetallicRoughness = 0x1,
	MaterialType_SpecularGlossiness = 0x2,
	MaterialType_Sheen = 0x4,
	MaterialType_ClearCoat = 0x8,
	MaterialType_Specular = 0x10,
	MaterialType_Transmission = 0x20,
	MaterialType_Volume = 0x40,
};

const uint32_t kMaxMaterials = 128;
const uint32_t kMaxEnvironments = 4;
const uint32_t kMaxLights = 8;

inline glm::mat4 ai_mat4_to_mat4(const aiMatrix4x4& from)
{
	glm::mat4 to;

	to[0][0] = (float)from.a1;
	to[0][1] = (float)from.b1;
	to[0][2] = (float)from.c1;
	to[0][3] = (float)from.d1;
	to[1][0] = (float)from.a2;
	to[1][1] = (float)from.b2;
	to[1][2] = (float)from.c2;
	to[1][3] = (float)from.d2;
	to[2][0] = (float)from.a3;
	to[2][1] = (float)from.b3;
	to[2][2] = (float)from.c3;
	to[2][3] = (float)from.d3;
	to[3][0] = (float)from.a4;
	to[3][1] = (float)from.b4;
	to[3][2] = (float)from.c4;
	to[3][3] = (float)from.d4;

	return to;
}

inline glm::vec3 ai_vector3D_to_vec3(const aiVector3D& from)
{
	return glm::vec3(from.x, from.y, from.z);
}

inline glm::quat ai_quaternion_to_quat(const aiQuaternion& from)
{
	return glm::quat(from.w, from.x, from.y, from.z);
}

enum LightType : uint32_t
{
	LightType_Directional = 0,
	LightType_Point = 1,
	LightType_Spot = 2,
};

struct EnvironmentMapDataGPU
{
	uint32_t envMapTexture = 0;
	uint32_t envMapTextureSampler = 0;
	uint32_t envMapTextureIrradiance = 0;
	uint32_t envMapTextureIrradianceSampler = 0;

	uint32_t lutBRDFTexture = 0;
	uint32_t lutBRDFTextureSampler = 0;

	uint32_t envMapTextureCharlie = 0;
	uint32_t envMapTextureCharlieSampler = 0;
};

struct LightDataGPU
{
	glm::vec3 direction = glm::vec3(0, 0, 1);
	float range = 10000.0;

	glm::vec3 color = glm::vec3(1, 1, 1);
	float intensity = 1.0;

	glm::vec3 position = glm::vec3(0, 0, -5);

	float innerConeCos = 0.0;
	float outerConeCos = 0.78;

	LightType type = LightType_Directional;

	int nodeId = ~0;
	int padding = 0;
};


struct GLTFGlobalSamplers
{
	VkSampler clamp;
	VkSampler wrap;
	VkSampler mirror;
};

struct EnvironmentsPerFrame {
	EnvironmentMapDataGPU environments[kMaxEnvironments];
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec4 color;
	glm::vec2 uv0;
	glm::vec2 uv1;
	float padding[2]; //to be investigated
};

//typedef struct MorphTarget
//{
//	uint32_t meshId;
//	std::vector<uint32_t> offset;
//} MorphTarget;

// static_assert(sizeof(Vertex) == sizeof(uint32_t) * 16);

struct GLTFMaterialTextures {
	//metallic roughness / specular glossiness
	VulkanTexture  baseColorTexture;
	VulkanTexture  surfacePropertiesTexture;

	//common
	VulkanTexture  normalTexture;
	VulkanTexture  occlusionTexture;
	VulkanTexture  emissiveTexture;

	//sheen
	VulkanTexture  sheenColorTexture;
	VulkanTexture  sheenRoughnessTexture;

	//clearcoat
	VulkanTexture  clearCoatTexture;
	VulkanTexture  clearCoatRoughnessTexture;
	VulkanTexture  clearCoatNormalTexture;

	//specular
	VulkanTexture  specularTexture;
	VulkanTexture  specularColorTexture;

	//transmission
	VulkanTexture  transmissionTexture;

	//volume
	VulkanTexture  thicknessTexture;

	//iridescene
	VulkanTexture  iridescenceTexture;
	VulkanTexture  iridescenceThicknessTexture;

	//anisotropy
	VulkanTexture  anisotropyTexture;
	VulkanTexture  white;
	bool wasLoaded;
};


struct EnvironmentMapTextures {
	VulkanTexture texBRDF_LUT;
	VulkanTexture envMapTexture;
	VulkanTexture envMapTextureCharlie;
	VulkanTexture envMapTextureIrradiance;
	VulkanTexture envMapSkybox;
};



struct GLTFMaterialDataGPU {
	glm::vec4 baseColorFactor;
	glm::vec4 metallicRoughnessNormalOcclusion;
	glm::vec4 specularGlossiness;
	glm::vec4 sheenFactors;
	glm::vec4 clearcoatTransmissionThickness;
	glm::vec4 specularFactors;
	glm::vec4 attenuation;
	glm::vec4 emissiveFactorAlphaCutoff;
	uint32_t occlusionTexture;
	uint32_t occlusionTextureSampler;
	uint32_t occlusionTextureUV;
	uint32_t emissiveTexture;
	uint32_t emissiveTextureSampler;
	uint32_t emissiveTextureUV;
	uint32_t baseColorTexture;
	uint32_t baseColorTextureSampler;
	uint32_t baseColorTextureUV;
	uint32_t surfacePropertiesTexture;
	uint32_t surfacePropertiesTextureSampler;
	uint32_t surfacePropertiesTextureUV;
	uint32_t normalTexture;
	uint32_t normalTextureSampler;
	uint32_t normalTextureUV;
	uint32_t sheenColorTexture;
	uint32_t sheenColorTextureSampler;
	uint32_t sheenColorTextureUV;
	uint32_t sheenRoughnessTexture;
	uint32_t sheenRoughnessTextureSampler;
	uint32_t sheenRoughnessTextureUV;
	uint32_t clearCoatTexture;
	uint32_t clearCoatTextureSampler;
	uint32_t clearCoatTextureUV;
	uint32_t clearCoatRoughnessTexture;
	uint32_t clearCoatRoughnessTextureSampler;
	uint32_t clearCoatRoughnessTextureUV;
	uint32_t clearCoatNormalTexture;
	uint32_t clearCoatNormalTextureSampler;
	uint32_t clearCoatNormalTextureUV;
	uint32_t specularTexture;
	uint32_t specularTextureSampler;
	uint32_t specularTextureUV;
	uint32_t specularColorTexture;
	uint32_t specularColorTextureSampler;
	uint32_t specularColorTextureUV;
	uint32_t transmissionTexture;
	uint32_t transmissionTextureSampler;
	uint32_t transmissionTextureUV;
	uint32_t thicknessTexture;
	uint32_t thicknessTextureSampler;
	uint32_t thicknessTextureUV;
	uint32_t iridescenceTexture;
	uint32_t iridescenceTextureSampler;
	uint32_t iridescenceTextureUV;
	uint32_t iridescenceThicknessTexture;
	uint32_t iridescenceThicknessTextureSampler;
	uint32_t iridescenceThicknessTextureUV;
	uint32_t anisotropyTexture;
	uint32_t anisotropyTextureSampler;
	uint32_t anisotropyTextureUV;
	uint32_t alphaMode;
	uint32_t materialTypeFlags;
	float ior;
	uint32_t padding[2];

	enum AlphaMode : uint32_t {
		AlphaMode_Opaque = 0,
		AlphaMode_Mask = 1,
		AlphaMode_Blend = 2,
	};
} ;

// static_assert(sizeof(GLTFMaterialDataGPU) % 16 == 0);

struct GLTFDataHolder
{
	struct GLTFContext* gltfContext;
	std::vector<GLTFMaterialTextures> textures;
};

struct MaterialsPerFrame {
	GLTFMaterialDataGPU materials[kMaxMaterials];
};

using GLTFNodeRef = uint32_t;
using GLTFMeshRef = uint32_t;


enum SortingType : uint32_t {
	SortingType_Opaque = 0,
	SortingType_Transmission = 1,
	SortingType_Transparent = 2,
};

struct GLTFMesh {
	VkPrimitiveTopology primitive;
	uint32_t vertexOffset;
	uint32_t vertexCount;
	uint32_t indexOffset;
	uint32_t indexCount;
	uint32_t matIdx;
	SortingType sortingType;
};


struct GLTFNode {
	std::string name;
	glm::mat4 transform;
	std::vector<GLTFNodeRef> children;
	std::vector<GLTFMeshRef> meshes;
	uint32_t modelMtxId;
};


struct GLTFFrameData {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 cameraPos;
};

struct GLTFCamera {
	std::string name;
	glm::vec3 pos;
	glm::vec3 up;
	uint32_t nodeIdx;
	glm::vec3 lookAt;
	float hFOV;
	float near;
	float far;
	float aspect;
	float orthoWidth;
};


struct GLTFTransforms {
	uint32_t modelMtxId;
	uint32_t matId;
	GLTFNodeRef nodeRef; // for CPU only
	GLTFMeshRef meshRef; // for CPU only
	uint32_t sortingType;
};

#define MAX_BONES_PER_VERTEX 8

struct VertexBoneData {
	glm::vec4 position;
	glm::vec4 normal;
	uint32_t boneId[MAX_BONES_PER_VERTEX];
	float weight[MAX_BONES_PER_VERTEX];
	uint32_t meshId;
};

// static_assert(sizeof(VertexBoneData) == sizeof(uint32_t) * 25);

struct GLTFBone {
	uint32_t boneId;
	glm::mat4 transform;
};

struct App;

struct GLTFContext {
	GLTFDataHolder glTFDataholder;
	MaterialsPerFrame matPerFrame;
	GLTFGlobalSamplers samplers;
	EnvironmentMapTextures envMapTextures;
	GLTFFrameData frameData;
	std::vector<GLTFTransforms> transforms;
	std::vector<glm::mat4> matrices;
	std::vector<GLTFNode> nodesStorage;
	std::vector<GLTFMesh> meshesStorage;
	std::unordered_map<std::string, GLTFBone> bonesByName;
	//std::vector<MorphTarget> morphTargets;
	std::unordered_map<std::string, uint32_t> meshesRemap;
	//std::vector<Animation> animations;
	std::vector<uint32_t> opaqueNodes;
	std::vector<uint32_t> transmissionNodes;
	std::vector<uint32_t> transparentNodes;


	
	VkPipelineLayout pipelineLayout; 

	// set 0 2D tex
	VkDescriptorSetLayout setLayout0;
	VkDescriptorSet bindlessSet0;

	// set 1 3D tex (placeholder)
	VkDescriptorSetLayout setLayout1;
	VkDescriptorSet dummySet1;

	// set 2 cube tex 
	VkDescriptorSetLayout setLayout2;
	VkDescriptorSet bindlessSet2;

	// set 3 shadow tex (placeholder)
	VkDescriptorSetLayout setLayout3;
	VkDescriptorSet dummySet3;

	VkDescriptorSetLayout setLayoutSamplers; // For kSamplers[]
	VkDescriptorSet bindlessSetSamplers;

	std::map<VkImageView, uint32_t> textureIndexMap;
	uint32_t nextTextureIndex;
	uint32_t nextCubemapIndex;
	std::map<VkImageView, uint32_t> cubemapIndexMap;

	VulkanBuffer envBuffer;
	VulkanBuffer lightsBuffer;
	VulkanBuffer perFrameBuffer;
	VulkanBuffer transformBuffer;
	VulkanBuffer matricesBuffer;
	//VulkanBuffer morphStatesBuffer;

	VkPipeline pipelineSolid_Pass1;
	VkPipeline pipelineTransparent_Pass1;
	VkPipeline pipelineSolid_Pass2;
	VkPipeline pipelineTransparent_Pass2;
	//VkPipelineLayout pipelineLayoutTransparent;
	//VkPipeline pipelineComputeAnimations;
	//VkPipelineLayout pipelineLayoutComputeAnimations;

	ShaderModule vert;
	ShaderModule frag;
	ShaderModule animation;
	VulkanBuffer vertexBuffer;
	VulkanBuffer vertexSkinningBuffer;
	VulkanBuffer vertexMorphingBuffer;
	VulkanBuffer indexBuffer;
	VulkanBuffer matBuffer;

	//VulkanTexture offscreenTex[3];
	//VkFramebuffer offscreenFb[3];
	//VkImageView offscreenFbView[3];
	//uint32_t currentOffscreenTex;

	VulkanTexture offscreenTex;
	VkImageView offscreenFbView;
	VkRenderPass offscreenRenderPass;
	VkRenderPass transparentRenderPass;
	VulkanTexture dummyWhite;

	uint32_t maxVertices;

	//std::vector<MorphState> morphStates;
	std::vector<LightDataGPU> lights;
	std::vector<GLTFCamera> cameras;

	GLTFIntrospective* inspector;

	VkDescriptorPool bindlessPool;
	VkDescriptorSetLayout bindlessLayout;
	VkDescriptorSet bindlessSet;

	GLTFNodeRef root;
	bool hasBones;
	bool isVolumetricMaterial;
	bool doublesided;

	bool enableMorphing;
	App* app; 
	LineCanvas3D* canvas3d;

	ShaderModule skyboxVert, skyboxFrag;
	VkPipeline skyboxPipeline;
	VkPipelineLayout skyboxPipelineLayout;

	bool renderSkyboxBackground = true;

	glm::vec3 averageEnvColor = glm::vec3(0.5f);
	int32_t selectedNodeIdx = -1;
	//bool animated;
	//bool skinning;
	//bool morphing;
};




void GLTFGlobalSamplers_init(GLTFGlobalSamplers* samplers, VulkanRenderDevice& vkDev);
void GLTFGlobalSamplers_destroy(GLTFGlobalSamplers* samplers, VkDevice device);
void EnvironmentMapTextures_init(GLTFContext* gltf);
void EnvironmentMapTextures_init_with_paths(
	GLTFContext* gltf, const char* brdfLUT, const char* prefilter,
	const char* irradiance, const char* prefilterCharlie);

void EnvironmentMapTextures_destroy(EnvironmentMapTextures* textures, VkDevice device);
glm::mat4 GLTFCamera_getProjection(const GLTFCamera* camera, float windowAspect = 1.0f);

void GLTFContext_init(GLTFContext* context, App* app);
void GLTFContext_destroy(GLTFContext* context);
bool GLTFContext_isScreenCopyRequired(const GLTFContext* context);



bool assignUVandSampler(
	const GLTFGlobalSamplers& samplers, const aiMaterial* mtlDescriptor, aiTextureType textureType, uint32_t& uvIndex,
	uint32_t& textureSampler, int index = 0);

GLTFMaterialDataGPU setupglTFMaterialData(
	GLTFContext* gltf, 
	const aiMaterial* mtlDescriptor, const char* assetFolder,
	bool& useVolumetric, bool& usedoublesided);
void loadGLTF(GLTFContext& gltf, const char* gltfName, const char* glTFDataPath);

void renderGLTF(
	GLTFContext& gltf,
	uint32_t currentSwapchainImageIndex,
	VkCommandBuffer buf,
	VkRenderPass mainRenderPass,    
	VkFramebuffer mainFramebuffer,  
	uint32_t swapchainWidth,
	uint32_t swapchainHeight,
	const glm::mat4& model,
	const glm::mat4& view,
	const glm::mat4& proj,
	bool rebuildRenderList = false);

//void animateGLTF(GLTFContext& gltf, AnimationState& anim, float dt);
//void animateBlendingGLTF(GLTFContext& gltf, AnimationState& anim1, AnimationState& anim2, float weight, float dt);
MaterialType detectMaterialType(const aiMaterial* mtl);

void printPrefix(int ofs);
void printMat4(const aiMatrix4x4& m);

std::vector<std::string> camerasGLTF(GLTFContext& context);
void updateCamera(GLTFContext& gltf, const glm::mat4& model, glm::mat4& view, glm::mat4& proj, float aspectRatio);

//std::vector<std::string> animationsGLTF(GLTFContext& gltf);


void buildTransformsList(GLTFContext& gltf);
void sortTransparentNodes(GLTFContext& gltf, const glm::vec3& cameraPos);
void updateLights(GLTFContext& gltf);

void renderEnvironmentCubemap(GLTFContext& gltf, VkCommandBuffer buf, const glm::mat4& proj, const glm::mat4& view);
void generate_BRDF_LUT(VulkanRenderDevice& vkDev, const char* outputPath);

void generate_prefiltered_envmap(GLTFContext& gltf, const char* inputHDR, const char* outputGGX, const char* outputCharlie, const char* outputIrradiance);
void prefilter_cubemap(GLTFContext* gltf, ktxTexture1* cube, const char* outputPath, VulkanTexture& srcEnvMap, uint32_t srcEnvMapBindlessIdx, Distribution distribution, uint32_t sampleCount);

void drawGizmo(GLTFContext& gltf);
void drawSelector(GLTFContext* gltf);
uint32_t get_texture_index(GLTFContext& gltf, const VulkanTexture& tex);