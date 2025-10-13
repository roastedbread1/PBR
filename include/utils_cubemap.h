#pragma once

#include <glm/glm.hpp>

#include <bitmap.h>

Bitmap convert_equirectangular_map_to_vertial_cross(const Bitmap& b);
Bitmap convert_vertical_cross_to_cubemap_faces(const Bitmap& b);

inline Bitmap convert_equirectangular_map_to_cubemap_faces(const Bitmap& b)
{
	return convert_vertical_cross_to_cubemap_faces(convert_equirectangular_map_to_vertial_cross(b));
}
glm::vec3 faceCoordsToXYZ(int i, int j, int faceID, int faceSize);
void convolve_diffuse(const glm::vec3* data, int srcW, int srcH, int dstW, int dstH, glm::vec3* out, int numMonteCarloSamples);