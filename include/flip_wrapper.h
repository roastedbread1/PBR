#pragma once

/*
FLIP image comparison wrapper
wraps NVIDIA's FLIP (header-only) for comparing native vs DLSS rendering
*/

#include <cstdint>
#include <vector>
#include <cstring>


struct FLIPCompareParams
{
	const float* referenceData;		// RGBA float pixels [0..1]
	const float* testData;			// RGBA float pixels [0..1]
	uint32_t width;
	uint32_t height;
	float pixelsPerDegree;			// viewing condition, 0 = use default (~67 for 1080p at 24")
};

struct FLIPResult
{
	float meanError;
	float maxError;
	float weightedMedian;
	std::vector<float> errorMap;	// per-pixel FLIP error [0..1], size = width * height
	uint32_t width;
	uint32_t height;
	bool valid;
};

struct FLIPContext
{
	std::vector<float> referenceBuffer;		// RGBA float
	std::vector<float> testBuffer;			// RGBA float
	uint32_t refWidth;
	uint32_t refHeight;
	uint32_t testWidth;
	uint32_t testHeight;

	FLIPResult lastResult;

	bool hasReference;
	bool hasTest;
	bool hasResult;
};

void flip_init(FLIPContext* ctx);
void flip_shutdown(FLIPContext* ctx);

bool flip_compare(FLIPContext* ctx, const FLIPCompareParams* params, FLIPResult* outResult);

void flip_capture_reference(FLIPContext* ctx, const float* rgbaData, uint32_t w, uint32_t h);
bool flip_capture_test_and_compare(FLIPContext* ctx, const float* rgbaData, uint32_t w, uint32_t h);

bool flip_has_result(const FLIPContext* ctx);
const FLIPResult* flip_get_result(const FLIPContext* ctx);

//NOTE: converts B8G8R8A8 sRGB pixels to linear float RGBA, caller owns the returned buffer
std::vector<float> flip_convert_bgra8_to_float(const uint8_t* bgraData, uint32_t w, uint32_t h);

//NOTE: converts R16G16B16A16 float pixels (sRGB/gamma-encoded) to linear float RGBA
std::vector<float> flip_convert_rgba16f_to_float(const uint16_t* rgba16fData, uint32_t w, uint32_t h);

//NOTE: converts linear float RGBA [0..1] to sRGB uint8 RGBA for display
std::vector<uint8_t> flip_float_to_rgba8(const float* rgbaData, uint32_t w, uint32_t h);

//NOTE: converts single-channel error map [0..1] to RGBA8 using magma colormap
std::vector<uint8_t> flip_error_map_to_magma_rgba8(const float* errorMap, uint32_t w, uint32_t h);


inline const char* flip_quality_label(float meanError)
{
	if (meanError < 0.05f)  return "Excellent";
	if (meanError < 0.10f)  return "Good";
	if (meanError < 0.20f)  return "Fair";
	if (meanError < 0.35f)  return "Poor";
	return "Bad";
}
