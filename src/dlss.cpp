#include <dlss.h>
#include <cmath>
#include <cstdio>
#include <cassert>

static NVSDK_NGX_PerfQuality_Value quality_mode_to_ngx(DLSSQualityMode mode)
{
	switch (mode)
	{
    case DLSSQualityMode::UltraPerformance:
        return NVSDK_NGX_PerfQuality_Value_UltraPerformance;
    case DLSSQualityMode::Performance:
        return NVSDK_NGX_PerfQuality_Value_MaxPerf;
    case DLSSQualityMode::Balanced:
        return NVSDK_NGX_PerfQuality_Value_Balanced;
    case DLSSQualityMode::Quality:
        return NVSDK_NGX_PerfQuality_Value_MaxQuality;
    case DLSSQualityMode::DLAA:
        return NVSDK_NGX_PerfQuality_Value_DLAA;
    default:
        return NVSDK_NGX_PerfQuality_Value_MaxQuality;
	}
}

static float halton(int index, int base)
{
    float f = 1.0f;
    float r = 0.0f;
    int curr = index;

    while (curr > 0)
    {
        f = f / static_cast<float>(base);
        r = r + f * static_cast<float>(curr % base);
        curr = curr / base;
    }

    return r;
}

bool dlss_init(DLSSContext* ctx, const DLSSInitParams* params)
{

    if (!ctx || !params) return false; //EWW early return

    *ctx = {};
    ctx->device = *params->device;
    ctx->physicalDevice = *params->physicalDevice;
    ctx->instance = *params->instance;
    ctx->currentMode = DLSSQualityMode::Off;
    {
       NVSDK_NGX_FeatureCommonInfo featureInfo = {};
       NVSDK_NGX_PathListInfo pathInfo = {};

       if (params->featureSearchPaths && params->featuresSearchPathCount > 0)
       {
          pathInfo.Path = const_cast<wchar_t**>(params->featureSearchPaths);
          pathInfo.Length = params->featuresSearchPathCount;
          featureInfo.PathListInfo = pathInfo;
          featureInfo.LoggingInfo.MinimumLoggingLevel = NVSDK_NGX_LOGGING_LEVEL_VERBOSE;
       }
       //TODO: figure out why its not triggering the # cpp (im not sure how this works, probably compiler flags) but the two nullptr should've been a default value
       NVSDK_NGX_Result result = NVSDK_NGX_VULKAN_Init_with_ProjectID(params->projectId, NVSDK_NGX_ENGINE_TYPE_CUSTOM, params->engineVersion, params->appDataPath,
         *params->instance, *params->physicalDevice, *params->device,nullptr,nullptr, &featureInfo, NVSDK_NGX_Version_API);
       printf("NGX_VULKAN_INIT:%s\n", dlss_result_to_string(result));
      if (NVSDK_NGX_FAILED(result))
      {
         printf("DLSS: NGX init failed: %s\n", dlss_result_to_string(result));
         return false;
      }
    
      ctx->initialized = true;

       result = NVSDK_NGX_VULKAN_GetCapabilityParameters(&ctx->ngxParams);
       printf("NVSDK_NGX_VULKAN_GetCapabilityParameters:%s\n", dlss_result_to_string(result));

      if (NVSDK_NGX_FAILED(result))
      {
         //NOTE: shouldve add a fallback but I think NVSDK_NGX_VULKAN_GetParameters is deprecated so it doesnt matter
           printf("DLSS: failed to get NGX params: %s\n", dlss_result_to_string(result));
           dlss_shutdown(ctx);
           //probably shoud just return true??? and just say due to some circumstances dlss cannot run
           return false;
      }
    }
    {

        int dlssAvailable = 0;

        NVSDK_NGX_Result ResultDlssSupported = ctx->ngxParams->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &dlssAvailable);
        if (NVSDK_NGX_FAILED(ResultDlssSupported) || !dlssAvailable)
        {
            printf("dlss is not available:0x%08X\n",dlssAvailable );
        }
        if (NVSDK_NGX_FAILED(ResultDlssSupported))
        {
            int  needsUpdate = 0;
            ctx->ngxParams->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needsUpdate);

            if (needsUpdate)
            {
                unsigned int minMajor = 0, minMinor = 0;
                ctx->ngxParams->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor, &minMajor);
                ctx->ngxParams->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor, &minMinor);
                printf("DLSS: Driver update required. Minimum version: %u.%u\n", minMajor, minMinor);
            }
            else
            {
                printf("DLSS not supported aka you're broke\n");
                
            }

            ctx->dlssAvailable = false;
            return true; 
        }
    }

    //checking for this because... i cant even begin to explain why, its so fucked
    // NOTE: moved this down, too lazy to delete felt like this was a rite a passage
    //{
    //    unsigned int dlssVersion = 0;
    //    if (ctx->ngxParams->Get("SuperSampling.FeatureVersion", &dlssVersion) == NVSDK_NGX_Result_Success)
    //    {
    //        printf("DLSS: Feature version 0x%X\n", dlssVersion);
    //    }

    //    
    //    void* callback = nullptr;
    //    NVSDK_NGX_Result cbResult = ctx->ngxParams->Get(NVSDK_NGX_Parameter_DLSSOptimalSettingsCallback, &callback);
    //    printf("DLSS: OptimalSettingsCallback query result: %s, callback=%p\n",
    //        dlss_result_to_string(cbResult), callback);
    //}

    {

        int featureInitResult = 0;
        ctx->ngxParams->Get(NVSDK_NGX_Parameter_SuperSampling_FeatureInitResult, &featureInitResult);

        if (featureInitResult == 0)
        {
            printf("DLSS: feature denied for this app\n");
            ctx->dlssAvailable = false;
            return true;
        }
    }

    ctx->dlssAvailable = true;
    printf("DLSS: initialized successfully\n");

    return true;
}



void dlss_shutdown(DLSSContext* ctx)
{
    if (!ctx) return;

    if (ctx->featureCreated && ctx->dlssFeature)
    {
        NVSDK_NGX_VULKAN_ReleaseFeature(ctx->dlssFeature);
        ctx->dlssFeature = nullptr;
        ctx->featureCreated = false;
    }

    if (ctx->ngxParams)
    {
        NVSDK_NGX_VULKAN_DestroyParameters(ctx->ngxParams);
        ctx->ngxParams = nullptr;
    }


    if (ctx->initialized)
    {
        NVSDK_NGX_VULKAN_Shutdown1(ctx->device);
        ctx->initialized = false;
    }

    ctx->dlssAvailable = false;
}
bool dlss_is_available(const DLSSContext* ctx)
{
    return ctx && ctx->initialized && ctx->dlssAvailable;
}
static bool manual_optimal_settings(uint32_t targetWidth, uint32_t targetHeight, DLSSQualityMode mode, DLSSOptimalSettings* outSettings)
{
    if (!outSettings) return false;

    *outSettings = {};
    float scaleFactor = 1.0f;


    switch (mode)
    {
    case DLSSQualityMode::DLAA:
        scaleFactor = 1.0f;
        break;
    case DLSSQualityMode::Quality:
        scaleFactor = 1.5f;
        break;
    case DLSSQualityMode::Balanced:
        scaleFactor = 1.724;
        break;
    case DLSSQualityMode::Performance:
        scaleFactor = 2.0f;
        break;
    case DLSSQualityMode::UltraPerformance:
        scaleFactor = 3.0f;
        break;
    default: 
        return false;
    }

    uint32_t renderWidth = static_cast<uint32_t>(targetWidth / scaleFactor);
    uint32_t renderHeight = static_cast<uint32_t>(targetHeight / scaleFactor);
    renderWidth = (renderWidth + 1) & ~1u;
    renderHeight = (renderHeight + 1) & ~1u;

    /*  from the docs
    Limitation: InTargetWidth and InTargetHeight currently have a limitation on their minimum size.They
    must be at least 64 pixels for the width and 32 pixels for the height.The call to Create or
    Evaluate the feature may fail as a result of this limitation.
    */
    renderWidth = std::max(renderWidth, 32u);
    renderHeight = std::max(renderHeight, 32u);



    outSettings->renderWidth = renderWidth;
    outSettings->renderHeight = renderHeight;
    outSettings->minRenderWidth = renderWidth;
    outSettings->minRenderHeight = renderHeight;
    outSettings->maxRenderWidth = renderWidth;
    outSettings->maxRenderHeight = renderHeight;
    outSettings->sharpness = 0.0f;
    outSettings->isSupported = true;

    printf("DLSS Fallback: %ux%u -> %ux%u (scale factor %.2f, mode %s)\n",
        targetWidth, targetHeight,
        renderWidth, renderHeight,
        scaleFactor,
        dlss_quality_mode_to_string(mode));

    return true;
}

bool dlss_query_optimal_settings(DLSSContext* ctx, uint32_t tWidth, uint32_t tHeight, DLSSQualityMode mode, DLSSOptimalSettings* outSettings)
{
    //TODO: find out if we should actually check for outsettings (i dont think so since we're clearing it anyway)
    if (!ctx || !ctx->ngxParams || mode == DLSSQualityMode::Off) return false;

    *outSettings = {};

    unsigned int optimalWidth = 0, optimalHeight = 0, minWidth = 0, minHeight = 0, maxWidth = 0, maxHeight = 0;
    float sharpness = 0.0f;
    void* callback = nullptr;
    
    NVSDK_NGX_Result cbResult = ctx->ngxParams->Get(NVSDK_NGX_Parameter_DLSSOptimalSettingsCallback, &callback);
    printf("DLSS: OptimalSettingsCallback query result: %s, callback=%p\n",
    dlss_result_to_string(cbResult), callback);
    
    if (callback != nullptr)
    {
        NVSDK_NGX_Result result = NGX_DLSS_GET_OPTIMAL_SETTINGS(ctx->ngxParams, tWidth, tHeight, quality_mode_to_ngx(mode), &optimalWidth, &optimalHeight,
            &maxWidth, &maxHeight, &minWidth, &minHeight, &sharpness);

        if (NVSDK_NGX_SUCCEED(result) && optimalWidth > 0 && optimalHeight > 0)
        {
            outSettings->renderWidth = optimalWidth;
            outSettings->renderHeight = optimalHeight;
            outSettings->minRenderWidth = minWidth;
            outSettings->minRenderHeight = minHeight;
            outSettings->maxRenderWidth = maxWidth;
            outSettings->maxRenderHeight = maxHeight;
            outSettings->sharpness = sharpness;
            outSettings->isSupported = true;

            printf("DLSS NGX: %ux%u -> render %ux%u\n", tWidth, tHeight, optimalWidth, optimalHeight);
            return true;
        }
    }
    
    

    //if (NVSDK_NGX_FAILED(result) || optimalWidth == 0 || optimalHeight == 0)
    //{
    //    printf("DLSS: Quality mode %s not supported for %ux%u (optimal: %ux%u)\n",
    //        dlss_quality_mode_to_string(mode), tWidth, tHeight, optimalWidth, optimalHeight);
    //    outSettings->isSupported = false;
    //    outSettings->isSupported = false;
    //    return false;
    //}


    printf("DLSS: Using fallback scaling ratios\n");
    return manual_optimal_settings(tWidth, tHeight, mode, outSettings);
}

static const char* dlss_get_preset_param_for_mode(DLSSQualityMode mode)
{
    switch (mode)
    {
    case DLSSQualityMode::DLAA:             return NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA;
    case DLSSQualityMode::Quality:          return NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality;
    case DLSSQualityMode::Balanced:         return NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced;
    case DLSSQualityMode::Performance:      return NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance;
    case DLSSQualityMode::UltraPerformance: return NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance;
    default:                                return NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality;
    }
}

bool dlss_create_feature(DLSSContext* ctx, VkCommandBuffer cmdBuffer, const DLSSFeatureParams* params)
{

    if(!ctx || !params|| !ctx->dlssAvailable)  return false;

    dlss_release_feature(ctx);

    DLSSOptimalSettings settings = {};
    if (!dlss_query_optimal_settings(ctx, params->targetWidth, params->targetHeight, params->mode, &settings))
    {
        printf("DLSS:  Quality mode not supported for resolution%ux%u\n", params->targetWidth, params->targetHeight);
        return false;
    }

    int featureFlags = NVSDK_NGX_DLSS_Feature_Flags_None;

    if (params->isHDR) featureFlags |= NVSDK_NGX_DLSS_Feature_Flags_IsHDR;

    if (params->motionVectorsLowRes) featureFlags |= NVSDK_NGX_DLSS_Feature_Flags_MVLowRes;

    if (params->motionVectorsJittered) featureFlags |= NVSDK_NGX_DLSS_Feature_Flags_MVJittered;

    if (params->depthInverted) featureFlags |= NVSDK_NGX_DLSS_Feature_Flags_DepthInverted;

    if (params->autoExposure) featureFlags |= NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;

    // Set render preset hint before feature creation
    const char* presetParam = dlss_get_preset_param_for_mode(params->mode);
    ctx->ngxParams->Set(presetParam, (unsigned int)params->dlssPreset);
    printf("DLSS: Setting preset %s for mode %s\n",
        dlss_preset_to_string(params->dlssPreset),
        dlss_quality_mode_to_string(params->mode));

    NVSDK_NGX_DLSS_Create_Params dlssCreateParams = {};
    dlssCreateParams.Feature.InWidth = settings.renderWidth;
    dlssCreateParams.Feature.InHeight = settings.renderHeight;
    dlssCreateParams.Feature.InTargetWidth = params->targetWidth;
    dlssCreateParams.Feature.InTargetHeight = params->targetHeight;
    dlssCreateParams.Feature.InPerfQualityValue = quality_mode_to_ngx(params->mode);
    dlssCreateParams.InFeatureCreateFlags = featureFlags;
    dlssCreateParams.InEnableOutputSubrects = false;

    NVSDK_NGX_Result result = NGX_VULKAN_CREATE_DLSS_EXT( cmdBuffer, 1, 1,  &ctx->dlssFeature, ctx->ngxParams, &dlssCreateParams);

    if (NVSDK_NGX_FAILED(result))
    {
        printf("failed to create DLSS: %s\n", dlss_result_to_string(result));
        return false;
    }

    ctx->featureCreated = true;
    ctx->featureFlags = featureFlags;
    ctx->currentMode = params->mode;
    ctx->currentPreset = params->dlssPreset;
    ctx->targetWidth = params->targetWidth;
    ctx->targetHeight = params->targetHeight;
    ctx->renderWidth = settings.renderWidth;
    ctx->renderHeight = settings.renderHeight;

    printf("DLSS feature created at : %ux%u with target : %ux%u (%s)\n", settings.renderWidth, settings.renderHeight,
        params->targetWidth, params->targetHeight, dlss_quality_mode_to_string(params->mode));

    return true;
}

void dlss_release_feature(DLSSContext* ctx)
{
    if (!ctx || !ctx->featureCreated || !ctx->dlssFeature) return;

    NVSDK_NGX_VULKAN_ReleaseFeature(ctx->dlssFeature); //NOTE: the command buffer that used this feature must have completed
    ctx->dlssFeature = nullptr;
    ctx->featureCreated = false;
}

bool dlss_evaluate(DLSSContext* ctx, VkCommandBuffer cmdBuffer, const DLSSEvalParams* params)
{
    if(!ctx || !params || !ctx->featureCreated  || !ctx->dlssFeature) return false;

    NVSDK_NGX_Resource_VK colorIn = NVSDK_NGX_Create_ImageView_Resource_VK(params->colorInput, params->colorInputImage,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_R16G16B16A16_SFLOAT, params->renderWidth, params->renderHeight, false);

    NVSDK_NGX_Resource_VK colorOut = NVSDK_NGX_Create_ImageView_Resource_VK( params->colorOutput, params->colorOutputImage,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_R16G16B16A16_SFLOAT, params->outputWidth, params->outputHeight, true );

    NVSDK_NGX_Resource_VK depth = NVSDK_NGX_Create_ImageView_Resource_VK( params->depthInput, params->depthInputImage,
        { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }, VK_FORMAT_D32_SFLOAT,  params->renderWidth, params->renderHeight, false );

    NVSDK_NGX_Resource_VK motionVectors = NVSDK_NGX_Create_ImageView_Resource_VK(params->motionVectors, params->motionVectorsImage,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }, VK_FORMAT_R16G16_SFLOAT, params->renderWidth, params->renderHeight, false);

    NVSDK_NGX_VK_DLSS_Eval_Params evalParams = {};
    evalParams.Feature.pInColor = &colorIn;
    evalParams.Feature.pInOutput = &colorOut;
    evalParams.Feature.InSharpness = 0.0f; // deprecated
    evalParams.pInDepth = &depth;
    evalParams.pInMotionVectors = &motionVectors;
    evalParams.InJitterOffsetX = params->jitterOffsetX;
    evalParams.InJitterOffsetY = params->jitterOffsetY;
    evalParams.InRenderSubrectDimensions.Width = params->renderWidth;
    evalParams.InRenderSubrectDimensions.Height = params->renderHeight;
    evalParams.InReset = params->reset ? 1 : 0;
    evalParams.InMVScaleX = params->motionVectorScaleX != 0.0f ? params->motionVectorScaleX : 1.0f;
    evalParams.InMVScaleY = params->motionVectorScaleY != 0.0f ? params->motionVectorScaleY : 1.0f;
    evalParams.InPreExposure = params->preExposure != 0.0f ? params->preExposure : 1.0f;

    NVSDK_NGX_Resource_VK exposure = {};
    if (params->exposureTexture != VK_NULL_HANDLE)
    {
        exposure = NVSDK_NGX_Create_ImageView_Resource_VK(
            params->exposureTexture,
            params->exposureTextureImage,
            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            VK_FORMAT_R16_SFLOAT,
            1, 1,
            false
        );
        evalParams.pInExposureTexture = &exposure;
    }

    NVSDK_NGX_Result result = NGX_VULKAN_EVALUATE_DLSS_EXT( cmdBuffer, ctx->dlssFeature, ctx->ngxParams, &evalParams);

    if (NVSDK_NGX_FAILED(result))
    {
        printf("DLSS failed: %s\n", dlss_result_to_string(result));
        return false;
    }

    return true;
}

void dlss_get_render_resolution(const DLSSContext* ctx, uint32_t* outWidth, uint32_t* outHeight)
{
    if (!ctx || !outWidth || !outHeight) return;

    *outWidth = ctx->renderWidth;
    *outHeight = ctx->renderHeight;
}

void dlss_get_jitter_offset(uint32_t frameIndex, uint32_t phaseCount, float* outX, float* outY)
{
    if (!outX || !outY || phaseCount == 0)
        return;

 
    int index = (frameIndex % phaseCount) + 1; 

 
    *outX = halton(index, 2) - 0.5f;
    *outY = halton(index, 3) - 0.5f;
}

uint32_t get_recommended_phase_count(DLSSQualityMode mode, uint32_t renderWidth, uint32_t targetWidth, float nativeBias)
{
    
    const uint32_t basePhaseCount = 8;

    if (mode == DLSSQualityMode::Off || mode == DLSSQualityMode::DLAA)
        return basePhaseCount;

    if (renderWidth == 0)
        return basePhaseCount;

    
    float scaleRatio = static_cast<float>(targetWidth) / static_cast<float>(renderWidth);
    uint32_t phaseCount = static_cast<uint32_t>(basePhaseCount * scaleRatio * scaleRatio);

    
    switch (mode)
    {
    case DLSSQualityMode::UltraPerformance:
        return phaseCount > 72 ? phaseCount : 72;
    case DLSSQualityMode::Performance:
        return phaseCount > 32 ? phaseCount : 32;
    case DLSSQualityMode::Balanced:
        return phaseCount > 24 ? phaseCount : 24;
    case DLSSQualityMode::Quality:
        return phaseCount > 18 ? phaseCount : 18;
    default:
        return phaseCount > 8 ? phaseCount : 8;
    }
}

//bool dlss_get_vram_usage(DLSSContext* ctx)
//{
//    if (!ctx || !ctx->ngxParams) return false;
//
//    unsigned long long vramUsage = 0;
//    PFN_NVSDK_NGX_DLSS_GetStatsCallback statsCallback = nullptr;
//    NVSDK_NGX_Result result = ctx->ngxParams->Get(NVSDK_NGX_Parameter_DLSSGetStatsCallback, reinterpret_cast<void**>(&statsCallback));
//
//    if (statsCallback)
//    {
//        statsCallback(&vramUsage);
//    }
//
//    return static_cast<uint64_t>(vramUsage);
//}

bool dlss_set_quality_mode(DLSSContext* ctx, VkCommandBuffer cmdBuffer, DLSSQualityMode mode, const DLSSFeatureParams* params)
{
    if (!ctx || !params) return false;

    if (mode == DLSSQualityMode::Off)
    {
        dlss_release_feature(ctx);
        ctx->currentMode = DLSSQualityMode::Off;
        return true;
    }

    DLSSFeatureParams newParams = *params;
    newParams.mode = mode;

    return dlss_create_feature(ctx, cmdBuffer, &newParams);
}



