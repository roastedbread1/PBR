#include <utils_math.h>
#include <utils_cubemap.h>

#include <cstdio>
#include <glm/glm.hpp>
#include <glm/ext.hpp>


#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

/// From Henry J. Warren's "Hacker's Delight"
float radicalInverse_VdC(uint32_t bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

/// The i-th point is then computed by

/// From http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

glm::vec2 hammersley2d(uint32_t i, uint32_t N)
{
    return glm::vec2(float(i) / float(N), radicalInverse_VdC(i));
}

Bitmap convert_equirectangular_map_to_vertial_cross(const Bitmap& b)
{

	if(b.type != eBitmapType_2D) return Bitmap();

	const int faceSize = b.w / 4;

	const int w = faceSize * 3;
	const int h = faceSize * 4;

	Bitmap result;
	init_bitmap(&result, w, h, b.comp, b.fmt);

	const glm::ivec2 kFaceOffsets[] =
	{
		glm::ivec2(faceSize, faceSize * 3),
		glm::ivec2(0, faceSize),
		glm::ivec2(faceSize, faceSize),
		glm::ivec2(faceSize * 2, faceSize),
		glm::ivec2(faceSize, 0),
		glm::ivec2(faceSize, faceSize * 2)
	};

	const int clampW = b.w - 1;
	const int clampH = b.h - 1;

	for (int face = 0; face != 6; face++)
	{
		for (int i = 0; i != faceSize; i++)
		{
			for (int j = 0; j != faceSize; j++)
			{
				const glm::vec3 P = faceCoordsToXYZ(i, j, face, faceSize);
				const float R = hypot(P.x, P.y);
				const float theta = atan2(P.y, P.x);
				const float phi = atan2(P.z, R);

				//FP source coord
				const float Uf = float(2.0f * faceSize * (theta + M_PI) / M_PI);
				const float Vf = float(2.0f * faceSize * (M_PI / 2.0f - phi) / M_PI);
				
				//4 samples for bilinear interp
				const int U1 = clamp(int(floor(Uf)), 0, clampW);
				const int V1 = clamp(int(floor(Vf)), 0, clampH);
				const int U2 = clamp(U1 + 1, 0, clampW);
				const int V2 = clamp(V1 + 1, 0, clampH);

				//frac
				const float s = Uf - U1;
				const float t = Vf - V1;
				
				//get 4 samples
				const glm::vec4 A = get_bitmap_pixel(&b, U1, V1);
				const glm::vec4 B = get_bitmap_pixel(&b, U2, V1);
				const glm::vec4 C = get_bitmap_pixel(&b, U1, V2);
				const glm::vec4 D = get_bitmap_pixel(&b, U2, V2);

				//bilinear interp
				const glm::vec4 color = A * (1 - s) * (1 - t) + B * (s) * (1 - t) + C * (1 - s) * t + D * (s) * (t);

				set_bitmap_pixel(&result, i + kFaceOffsets[face].x, j + kFaceOffsets[face].y, color);

				
			}
		}
	}

	return result;
}

Bitmap convert_vertical_cross_to_cubemap_faces(const Bitmap& b)
{

	const int faceWidth = b.w / 3;
	const int faceHeight = b.h / 4;

	Bitmap cubemap;
	init_bitmap_3d(&cubemap, faceWidth, faceHeight, 6, b.comp, b.fmt);
	cubemap.type = eBitmapType_Cube;

	const uint8_t* src = b.data.data();
	uint8_t* dst = cubemap.data.data();


		/*
			------
			| +Y |
		----------------
		| -X | -Z | +X |
		----------------
			| -Y |
			------
			| +Z |
			------
		*/

	const int pixelSize = cubemap.comp * bitmap_get_bytes_per_component(cubemap.fmt);

	for (int face = 0; face != 6; ++face)
	{
		for (int j = 0; j != faceHeight; ++j)
		{
			for (int i = 0; i != faceWidth; ++i)
			{
				int x = 0;
				int y = 0;

				switch(face)
				{
					// GL_TEXTURE_CUBE_MAP_POSITIVE_X
				case 0:
					x = i;
					y = faceHeight + j;
				
				break;
				// GL_TEXTURE_CUBE_MAP_NEGATIVE_X
				case 1:
					x = 2 * faceWidth + i;
					y = 1 * faceHeight + j;
			

				break;
				// GL_TEXTURE_CUBE_MAP_POSITIVE_Y
				case 2:
					x = 2 * faceWidth - (i + 1);
					y = 1 * faceHeight - (j + 1);
					
				break;
				// GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
				case 3:
					x = 2 * faceWidth - (i + 1);
					y = 3 * faceHeight - (j + 1);
				
				break;
				// GL_TEXTURE_CUBE_MAP_POSITIVE_Z
				case 4:
					x = 2 * faceWidth - (i + 1);
					y = b.h - (j + 1);
				
				break;
				// GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
				case 5:
					x = faceWidth + i;
					y = faceHeight + j;
				
				break;
				}

				memcpy(dst, src + (y * b.w + x) * pixelSize, pixelSize);

				dst += pixelSize;
			}
		}
	}

	return cubemap;
}

void convolve_diffuse(const glm::vec3* data, int srcW, int srcH, int dstW, int dstH, glm::vec3* out, int numMonteCarloSamples)
{
    assert(srcW == 2 * srcH);

    if (srcW != 2 * srcH) return;

    std::vector<glm::vec3> temp(dstW * dstH);

    stbir_resize(reinterpret_cast<const float*>(data),
        srcW,
        srcH,
        0,
        reinterpret_cast<float*>(temp.data()),
        dstW,
        dstH,
        0,
        STBIR_RGB,
        STBIR_TYPE_FLOAT,
        STBIR_EDGE_CLAMP,
        STBIR_FILTER_CUBICBSPLINE);

    const glm::vec3* scratch = temp.data();
    srcW = dstW;
    srcH = dstH;


	for (int y = 0; y != dstH; y++)
	{
		printf("Line %i...\n", y);
		const float theta1 = float(y) / float(dstH) * MATH_PI;
		for (int x = 0; x != dstW; x++)
		{
			const float phi1 = float(x) / float(dstW) * MATH_TWOPI;
			const glm::vec3 V1 = glm::vec3(sin(theta1) * cos(phi1), sin(theta1) * sin(phi1), cos(theta1));
			glm::vec3 color = glm::vec3(0.0f);
			float weight = 0.0f;
			for (int i = 0; i != numMonteCarloSamples; i++)
			{
				const glm::vec2 h = hammersley2d(i, numMonteCarloSamples);
				const int x1 = int(floor(h.x * srcW));
				const int y1 = int(floor(h.y * srcH));
				const float theta2 = float(y1) / float(srcH) * MATH_PI;
				const float phi2 = float(x1) / float(srcW) * MATH_TWOPI;
				const glm::vec3 V2 = glm::vec3(sin(theta2) * cos(phi2), sin(theta2) * sin(phi2), cos(theta2));
				const float D = std::max(0.0f, glm::dot(V1, V2));
				if (D > 0.01f)
				{
					color += scratch[y1 * srcW + x1] * D;
					weight += D;
				}
			}
			out[y * dstW + x] = color / weight;
		}
	}
}

glm::vec3 faceCoordsToXYZ(int i, int j, int faceID, int faceSize)
{
	const float A = 2.0f * float(i) / faceSize;
	const float B = 2.0f * float(j) / faceSize;

	if (faceID == 0) return glm::vec3(-1.0f, A - 1.0f, B - 1.0f);
	if (faceID == 1) return glm::vec3(A - 1.0f, -1.0f, 1.0f - B);
	if (faceID == 2) return glm::vec3(1.0f, A - 1.0f, 1.0f - B);
	if (faceID == 3) return glm::vec3(1.0f - A, 1.0f, 1.0f - B);
	if (faceID == 4) return glm::vec3(B - 1.0f, A - 1.0f, 1.0f);
	if (faceID == 5) return glm::vec3(1.0f - B, A - 1.0f, -1.0f);

	return glm::vec3();
}


