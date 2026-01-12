#pragma once

/*
DLLS implementation prototype, will be testing to see if it works or not 
*/

#include <volk.h>
#include <nvsdk_ngx.h>
#include <nvsdk_ngx_vk.h>
#include <nvsdk_ngx_helpers.h>
#include <nvsdk_ngx_helpers_vk.h>

#include <string>
#include <functional>


struct VulkanRenderDevice;
struct VulkanInstance;
struct VulkanTexture;

enum DLSSQualityMode : uint32_t
{
	Off = 0,
	UltraPerformance = 1,
	Performance = 2,
	Balanced = 3,
	Quality = 4,
	DLAA = 5,
};

struct DLSSInitParams
{
	VkInstance* instance;
	VkPhysicalDevice* physicalDevice;
	VkDevice* device;

	const char* projectId;
	const char* engineVersion;
	const wchar_t* appDataPath; //log writes
	const wchar_t** featureSearchPaths;
	uint32_t featuresSearchPathCount;
};


struct DLSSFeatureParams
{
	uint32_t targetWidth;
	uint32_t targetHeight;
	DLSSQualityMode mode;

	bool isHDR;
	bool motionVectorsLowRes;
	bool motionVectorsJittered;
	bool depthInverted;
	bool autoExposure;
	bool alphaUpscaling;
};

struct DLSSEvalParams
{
	VkImageView colorInput;
	VkImage colorInputImage;
	VkImageView colorOutput;
	VkImage colorOutputImage;
	VkImageView depthInput;
	VkImage depthInputImage;
	VkImageView motionVectors;
	VkImage motionVectorsImage;

	uint32_t renderWidth;
	uint32_t renderHeight;
	uint32_t outputWidth;
	uint32_t outputHeight;

	float jitterOffsetX;
	float jitterOffsetY;

	float motionVectorScaleX;
	float motionVectorScaleY;

	VkImageView exposureTexture;
	VkImage exposureTextureImage;

	float preExposure;

	bool reset;
};

struct DLSSOptimalSettings
{
	uint32_t            renderWidth;
	uint32_t            renderHeight;
	uint32_t            minRenderWidth;     
	uint32_t            minRenderHeight;
	uint32_t            maxRenderWidth;
	uint32_t            maxRenderHeight;
	float               sharpness;          // deprecated
	bool                isSupported;
};


struct DLSSContext
{
	NVSDK_NGX_Parameter* ngxParams;
	NVSDK_NGX_Handle* dlssFeature;
	DLSSQualityMode         currentMode;
	uint32_t                targetWidth;
	uint32_t                targetHeight;
	uint32_t                renderWidth;
	uint32_t                renderHeight;
	int                     featureFlags;
	bool                    initialized;
	bool                    featureCreated;
	bool                    dlssAvailable;
	VkDevice                device;
	VkPhysicalDevice        physicalDevice;
	VkInstance              instance;
};

bool dlss_init(DLSSContext* ctx, const DLSSInitParams* params);
void dlss_shutdown(DLSSContext* ctx);
bool dlss_is_available(const DLSSContext* ctx);
bool dlss_query_optimal_settings(DLSSContext* ctx, uint32_t tWidth, uint32_t tHeight, DLSSQualityMode mode, DLSSOptimalSettings* outSettings);
bool dlss_create_feature(DLSSContext* ctx, VkCommandBuffer cmdBuffer, const DLSSFeatureParams* params);
void dlss_release_feature(DLSSContext* ctx);
bool dlss_evaluate(DLSSContext* ctx, VkCommandBuffer cmdBuffer, const DLSSEvalParams* params);
void dlss_get_render_resolution(const DLSSContext* ctx, uint32_t* outWidth, uint32_t* outHeight);
//bool dlss_get_vram_usage(DLSSContext* ctx);
void dlss_get_jitter_offset(uint32_t frameIndex, uint32_t phaseCount, float* outX, float* outY);
uint32_t get_recommended_phase_count(DLSSQualityMode mode, uint32_t renderWidth, uint32_t targetWidth, float nativeBias);


//inline const char* glss_get_version_string()
//{
//	return "310.4.0";
//};

inline const char* dlss_quality_mode_to_string(DLSSQualityMode mode)
{
	switch (mode)
	{
	case DLSSQualityMode::Off:              return "Off";
	case DLSSQualityMode::UltraPerformance: return "Ultra Performance";
	case DLSSQualityMode::Performance:      return "Performance";
	case DLSSQualityMode::Balanced:         return "Balanced";
	case DLSSQualityMode::Quality:          return "Quality";
	case DLSSQualityMode::DLAA:             return "DLAA";
	default:                                return "Unknown";
	}
}
inline const char* dlss_result_to_string(NVSDK_NGX_Result result)
{
	switch (result)
	{
	case NVSDK_NGX_Result_Success:
		return "Success";
	case NVSDK_NGX_Result_FAIL_FeatureNotSupported:
		return "Feature not supported";
	case NVSDK_NGX_Result_FAIL_PlatformError:
		return "Platform error";
	case NVSDK_NGX_Result_FAIL_FeatureAlreadyExists:
		return "Feature already exists";
	case NVSDK_NGX_Result_FAIL_FeatureNotFound:
		return "Feature not found";
	case NVSDK_NGX_Result_FAIL_InvalidParameter:
		return "Invalid parameter";
	case NVSDK_NGX_Result_FAIL_ScratchBufferTooSmall:
		return "Scratch buffer too small";
	case NVSDK_NGX_Result_FAIL_NotInitialized:
		return "Not initialized";
	case NVSDK_NGX_Result_FAIL_UnsupportedInputFormat:
		return "Unsupported input format";
	case NVSDK_NGX_Result_FAIL_RWFlagMissing:
		return "RW flag missing";
	case NVSDK_NGX_Result_FAIL_MissingInput:
		return "Missing input";
	case NVSDK_NGX_Result_FAIL_UnableToInitializeFeature:
		return "Unable to initialize feature";
	case NVSDK_NGX_Result_FAIL_OutOfDate:
		return "Out of date";
	case NVSDK_NGX_Result_FAIL_OutOfGPUMemory:
		return "Out of GPU memory";
	case NVSDK_NGX_Result_FAIL_UnsupportedFormat:
		return "Unsupported format";
	case NVSDK_NGX_Result_FAIL_UnableToWriteToAppDataPath:
		return "Unable to write to app data path";
	case NVSDK_NGX_Result_FAIL_UnsupportedParameter:
		return "Unsupported parameter";
	case NVSDK_NGX_Result_FAIL_Denied:
		return "Denied";
	default:
		return "Unknown error";
	}
}


