#include <flip_wrapper.h>
#include <FLIP.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <numeric>


static float srgb_to_linear(float s)
{
	if (s <= 0.04045f)
		return s / 12.92f;
	return std::pow((s + 0.055f) / 1.055f, 2.4f);
}

static float half_to_float(uint16_t h)
{
	uint32_t sign = (h >> 15) & 0x1;
	uint32_t exponent = (h >> 10) & 0x1F;
	uint32_t mantissa = h & 0x3FF;

	if (exponent == 0)
	{
		if (mantissa == 0)
			return sign ? -0.0f : 0.0f;
		// denormalized
		float val = std::ldexp(static_cast<float>(mantissa), -24);
		return sign ? -val : val;
	}

	if (exponent == 31)
	{
		if (mantissa == 0)
			return sign ? -INFINITY : INFINITY;
		return NAN;
	}

	float val = std::ldexp(static_cast<float>(mantissa + 1024), static_cast<int>(exponent) - 25);
	return sign ? -val : val;
}

static float default_ppd()
{
	const float monitorDistance = 0.7f;		
	const float monitorWidth = 0.5268f;		
	const float monitorResX = 1920.0f;
	return monitorDistance * (monitorResX / monitorWidth) * (3.14159265f / 180.0f);
}


void flip_init(FLIPContext* ctx)
{
	if (!ctx) return;

	*ctx = {};
	ctx->hasReference = false;
	ctx->hasTest = false;
	ctx->hasResult = false;
	ctx->lastResult.valid = false;

	printf("FLIP: initialized\n");
}

void flip_shutdown(FLIPContext* ctx)
{
	if (!ctx) return;

	ctx->referenceBuffer.clear();
	ctx->testBuffer.clear();
	ctx->lastResult.errorMap.clear();
	ctx->lastResult.valid = false;
	ctx->hasReference = false;
	ctx->hasTest = false;
	ctx->hasResult = false;

	printf("FLIP: shutdown\n");
}

bool flip_compare(FLIPContext* ctx, const FLIPCompareParams* params, FLIPResult* outResult)
{
	if (!ctx || !params || !outResult) return false;
	if (!params->referenceData || !params->testData) return false;
	if (params->width == 0 || params->height == 0) return false;

	uint32_t w = params->width;
	uint32_t h = params->height;
	float ppd = params->pixelsPerDegree > 0.0f ? params->pixelsPerDegree : default_ppd();

	printf("FLIP: comparing %ux%u images (ppd=%.1f)\n", w, h, ppd);

	FLIP::image<FLIP::color3> referenceImage(w, h);
	FLIP::image<FLIP::color3> testImage(w, h);

	for (uint32_t y = 0; y < h; y++)
	{
		for (uint32_t x = 0; x < w; x++)
		{
			uint32_t idx = (y * w + x) * 4;
			referenceImage.set(x, y, FLIP::color3(
				params->referenceData[idx + 0],
				params->referenceData[idx + 1],
				params->referenceData[idx + 2]));
			testImage.set(x, y, FLIP::color3(
				params->testData[idx + 0],
				params->testData[idx + 1],
				params->testData[idx + 2]));
		}
	}

	FLIP::image<float> errorMap(w, h, 0.0f);

	FLIP::Parameters flipParams;
	flipParams.PPD = ppd;

	
	FLIP::evaluate(referenceImage, testImage, false, flipParams, errorMap);

	outResult->width = w;
	outResult->height = h;
	outResult->errorMap.resize(w * h);

	float sum = 0.0f;
	float maxErr = 0.0f;

	for (uint32_t y = 0; y < h; y++)
	{
		for (uint32_t x = 0; x < w; x++)
		{
			float e = errorMap.get(x, y);
			outResult->errorMap[y * w + x] = e;
			sum += e;
			maxErr = std::max(maxErr, e);
		}
	}

	uint32_t totalPixels = w * h;
	outResult->meanError = sum / static_cast<float>(totalPixels);
	outResult->maxError = maxErr;


	{
		std::vector<float> sorted(outResult->errorMap.begin(), outResult->errorMap.end());
		std::sort(sorted.begin(), sorted.end());
		outResult->weightedMedian = sorted[totalPixels / 2];
	}

	outResult->valid = true;

	printf("FLIP: mean=%.4f max=%.4f median=%.4f (%s)\n",
		outResult->meanError, outResult->maxError, outResult->weightedMedian,
		flip_quality_label(outResult->meanError));

	return true;
}

void flip_capture_reference(FLIPContext* ctx, const float* rgbaData, uint32_t w, uint32_t h)
{
	if (!ctx || !rgbaData || w == 0 || h == 0) return;

	size_t size = static_cast<size_t>(w) * h * 4;
	ctx->referenceBuffer.resize(size);
	memcpy(ctx->referenceBuffer.data(), rgbaData, size * sizeof(float));
	ctx->refWidth = w;
	ctx->refHeight = h;
	ctx->hasReference = true;
	ctx->hasResult = false;

	printf("FLIP: captured reference %ux%u\n", w, h);
}

bool flip_capture_test_and_compare(FLIPContext* ctx, const float* rgbaData, uint32_t w, uint32_t h)
{
	if (!ctx || !rgbaData || w == 0 || h == 0) return false;

	if (!ctx->hasReference)
	{
		printf("FLIP: no reference captured, capture reference first\n");
		return false;
	}

	if (ctx->refWidth != w || ctx->refHeight != h)
	{
		printf("FLIP: size mismatch - reference %ux%u vs test %ux%u\n",
			ctx->refWidth, ctx->refHeight, w, h);
		return false;
	}

	size_t size = static_cast<size_t>(w) * h * 4;
	ctx->testBuffer.resize(size);
	memcpy(ctx->testBuffer.data(), rgbaData, size * sizeof(float));
	ctx->testWidth = w;
	ctx->testHeight = h;
	ctx->hasTest = true;

	FLIPCompareParams params = {};
	params.referenceData = ctx->referenceBuffer.data();
	params.testData = ctx->testBuffer.data();
	params.width = w;
	params.height = h;
	params.pixelsPerDegree = 0.0f; // use default

	bool ok = flip_compare(ctx, &params, &ctx->lastResult);
	ctx->hasResult = ok;
	return ok;
}

bool flip_has_result(const FLIPContext* ctx)
{
	return ctx && ctx->hasResult && ctx->lastResult.valid;
}

const FLIPResult* flip_get_result(const FLIPContext* ctx)
{
	if (!ctx || !ctx->hasResult) return nullptr;
	return &ctx->lastResult;
}

std::vector<float> flip_convert_bgra8_to_float(const uint8_t* bgraData, uint32_t w, uint32_t h)
{
	size_t pixelCount = static_cast<size_t>(w) * h;
	std::vector<float> result(pixelCount * 4);

	for (size_t i = 0; i < pixelCount; i++)
	{
	
		float b = srgb_to_linear(bgraData[i * 4 + 0] / 255.0f);
		float g = srgb_to_linear(bgraData[i * 4 + 1] / 255.0f);
		float r = srgb_to_linear(bgraData[i * 4 + 2] / 255.0f);
		float a = bgraData[i * 4 + 3] / 255.0f;

		result[i * 4 + 0] = r;
		result[i * 4 + 1] = g;
		result[i * 4 + 2] = b;
		result[i * 4 + 3] = a;
	}

	return result;
}

std::vector<float> flip_convert_rgba16f_to_float(const uint16_t* rgba16fData, uint32_t w, uint32_t h)
{
	size_t pixelCount = static_cast<size_t>(w) * h;
	std::vector<float> result(pixelCount * 4);

	for (size_t i = 0; i < pixelCount; i++)
	{
		float r = half_to_float(rgba16fData[i * 4 + 0]);
		float g = half_to_float(rgba16fData[i * 4 + 1]);
		float b = half_to_float(rgba16fData[i * 4 + 2]);
		float a = half_to_float(rgba16fData[i * 4 + 3]);

		result[i * 4 + 0] = srgb_to_linear(r);
		result[i * 4 + 1] = srgb_to_linear(g);
		result[i * 4 + 2] = srgb_to_linear(b);
		result[i * 4 + 3] = a;
	}

	return result;
}

static float linear_to_srgb(float l)
{
	if (l <= 0.0f) return 0.0f;
	if (l >= 1.0f) return 1.0f;
	if (l <= 0.0031308f)
		return l * 12.92f;
	return 1.055f * std::pow(l, 1.0f / 2.4f) - 0.055f;
}

std::vector<uint8_t> flip_float_to_rgba8(const float* rgbaData, uint32_t w, uint32_t h)
{
	size_t pixelCount = static_cast<size_t>(w) * h;
	std::vector<uint8_t> result(pixelCount * 4);

	for (size_t i = 0; i < pixelCount; i++)
	{
		float r = linear_to_srgb(rgbaData[i * 4 + 0]);
		float g = linear_to_srgb(rgbaData[i * 4 + 1]);
		float b = linear_to_srgb(rgbaData[i * 4 + 2]);
		float a = std::min(std::max(rgbaData[i * 4 + 3], 0.0f), 1.0f);

		result[i * 4 + 0] = static_cast<uint8_t>(r * 255.0f + 0.5f);
		result[i * 4 + 1] = static_cast<uint8_t>(g * 255.0f + 0.5f);
		result[i * 4 + 2] = static_cast<uint8_t>(b * 255.0f + 0.5f);
		result[i * 4 + 3] = static_cast<uint8_t>(a * 255.0f + 0.5f);
	}

	return result;
}

// magma colormap LUT (16 stops, from matplotlib/FLIP)
static const float kMagmaLUT[][3] = {
	{ 0.001f, 0.000f, 0.014f },
	{ 0.064f, 0.030f, 0.180f },
	{ 0.143f, 0.047f, 0.325f },
	{ 0.238f, 0.040f, 0.437f },
	{ 0.335f, 0.055f, 0.489f },
	{ 0.424f, 0.095f, 0.497f },
	{ 0.510f, 0.140f, 0.491f },
	{ 0.600f, 0.179f, 0.475f },
	{ 0.692f, 0.214f, 0.441f },
	{ 0.776f, 0.259f, 0.390f },
	{ 0.847f, 0.322f, 0.326f },
	{ 0.906f, 0.399f, 0.258f },
	{ 0.949f, 0.493f, 0.183f },
	{ 0.975f, 0.601f, 0.107f },
	{ 0.985f, 0.724f, 0.068f },
	{ 0.987f, 0.991f, 0.644f },
};
static const int kMagmaLUTSize = 16;

std::vector<uint8_t> flip_error_map_to_magma_rgba8(const float* errorMap, uint32_t w, uint32_t h)
{
	size_t pixelCount = static_cast<size_t>(w) * h;
	std::vector<uint8_t> result(pixelCount * 4);

	for (size_t i = 0; i < pixelCount; i++)
	{
		float e = std::min(std::max(errorMap[i], 0.0f), 1.0f);
		float t = e * (kMagmaLUTSize - 1);
		int idx = static_cast<int>(t);
		float frac = t - idx;
		if (idx >= kMagmaLUTSize - 1) { idx = kMagmaLUTSize - 2; frac = 1.0f; }

		float r = kMagmaLUT[idx][0] + frac * (kMagmaLUT[idx + 1][0] - kMagmaLUT[idx][0]);
		float g = kMagmaLUT[idx][1] + frac * (kMagmaLUT[idx + 1][1] - kMagmaLUT[idx][1]);
		float b = kMagmaLUT[idx][2] + frac * (kMagmaLUT[idx + 1][2] - kMagmaLUT[idx][2]);

		result[i * 4 + 0] = static_cast<uint8_t>(r * 255.0f + 0.5f);
		result[i * 4 + 1] = static_cast<uint8_t>(g * 255.0f + 0.5f);
		result[i * 4 + 2] = static_cast<uint8_t>(b * 255.0f + 0.5f);
		result[i * 4 + 3] = 255;
	}

	return result;
}
