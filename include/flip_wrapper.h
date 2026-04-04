#pragma once


#include <cstdint>
#include <vector>
#include <cstring>


struct FLIPCompareParams
{
	const float* referenceData;		
	const float* testData;			
	uint32_t width;
	uint32_t height;
	float pixelsPerDegree;			
};

struct FLIPResult
{
	float meanError;
	float maxError;
	float weightedMedian;
	std::vector<float> errorMap;	
	uint32_t width;
	uint32_t height;
	bool valid;
};

struct FLIPContext
{
	std::vector<float> referenceBuffer;		
	std::vector<float> testBuffer;			
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

std::vector<float> flip_convert_bgra8_to_float(const uint8_t* bgraData, uint32_t w, uint32_t h);

std::vector<float> flip_convert_rgba16f_to_float(const uint16_t* rgba16fData, uint32_t w, uint32_t h);

std::vector<uint8_t> flip_float_to_rgba8(const float* rgbaData, uint32_t w, uint32_t h);

std::vector<uint8_t> flip_error_map_to_magma_rgba8(const float* errorMap, uint32_t w, uint32_t h);


inline const char* flip_quality_label(float meanError)
{
	if (meanError < 0.05f)  return "Excellent";
	if (meanError < 0.10f)  return "Good";
	if (meanError < 0.20f)  return "Fair";
	if (meanError < 0.35f)  return "Poor";
	return "Bad";
}
