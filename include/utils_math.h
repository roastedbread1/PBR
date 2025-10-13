#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <vector>
#include <limits>
#include <cstdlib>

#define MATH_PI 3.14159265359f;
#define MATH_TWOPI 6.28318530718f;

struct BoundingBox
{
	glm::vec3 min;
	glm::vec3 max;
 };

void init_bbox(BoundingBox* bbox, const glm::vec3& min, const glm::vec3& max);
void init_bbox_from_points(BoundingBox* bbox, const glm::vec3* points, size_t numPoints);
glm::vec3  get_bbox_size(const BoundingBox* bbox);
glm::vec3 get_bbox_center(const BoundingBox* bbox);
void transform_bbox(BoundingBox* bbox, const glm::mat4& t);
BoundingBox get_transformed_bbox(const BoundingBox* bbox, const glm::mat4& t);
void combine_bbox_point(BoundingBox* bbox, const glm::vec3& p);


template <typename T>
T clamp(T v, T a, T b);

float random01();
float random_float(float min, float max);
glm::vec3 random_vec(const glm::vec3& min, const glm::vec3& max);
glm::vec3 rand_vec();

void get_frustum_planes(glm::mat4 mvp, glm::vec4 * planes);
void get_frustum_corners(glm::mat4 mvp, glm::vec4* points);
bool is_box_in_frustum(glm::vec4* frustumPlanes, glm::vec4* frustumCorners, const BoundingBox* bbox);
BoundingBox combines_boxes(const std::vector<BoundingBox>& boxes);

template <typename T>
T clamp(T v, T a, T b)
{
	if (v < a) return a;
	if (v > b) return b;
	return v;
}

inline float random01()
{
	return (float)rand() / (float)RAND_MAX;
}

inline float random_float(float min, float max)
{
	return min + (max - min) * random01();
}

inline glm::vec3 random_vec(const glm::vec3& min, const glm::vec3& max)
{
	return glm::vec3(random_float(min.x, max.x), random_float(min.y, max.y), random_float(min.z, max.z));
}

inline glm::vec3 rand_vec()
{
	return random_vec(glm::vec3(-5, -5, -5), glm::vec3(5, 5, 5));
}

inline void init_bbox(BoundingBox* bbox, const glm::vec3& min, const glm::vec3& max)
{
	bbox->min = glm::min(min, max);
	bbox->max = glm::max(min, max);
}

inline void init_bbox_from_points(BoundingBox* bbox, const glm::vec3* points, size_t numPoints)
{
	glm::vec3 vmin(std::numeric_limits<float>::max());
	glm::vec3 vmax(std::numeric_limits<float>::lowest());

	for (size_t i = 0; i != numPoints; i++)
	{
		vmin = glm::min(vmin, points[i]);
		vmax = glm::max(vmax, points[i]);
	}
	bbox->min = vmin;
	bbox->max = vmax;
}

inline glm::vec3  get_bbox_size(const BoundingBox* bbox)
{
	return glm::vec3(bbox->max[0] - bbox->min[0], bbox->max[1] - bbox->min[1], bbox->max[2] - bbox->min[2]);
}

inline glm::vec3 get_bbox_center(const BoundingBox* bbox)
{
	return 0.5f * glm::vec3(bbox->max[0] + bbox->min[0], bbox->max[1] + bbox->min[1], bbox->max[2] + bbox->min[2]);
}

inline void transform_bbox(BoundingBox* bbox, const glm::mat4& t)
{
	glm::vec3 corners[] = 
	{
		glm::vec3(bbox->min.x, bbox->min.y, bbox->min.z),
		glm::vec3(bbox->min.x, bbox->max.y, bbox->min.z),
		glm::vec3(bbox->min.x, bbox->min.y, bbox->max.z),
		glm::vec3(bbox->min.x, bbox->max.y, bbox->max.z),
		glm::vec3(bbox->max.x, bbox->min.y, bbox->min.z),
		glm::vec3(bbox->max.x, bbox->max.y, bbox->min.z),
		glm::vec3(bbox->max.x, bbox->min.y, bbox->max.z),
		glm::vec3(bbox->max.x, bbox->max.y, bbox->max.z),
	};

	for (auto& v : corners)
		v = glm::vec3(t * glm::vec4(v, 1.0f));
	init_bbox_from_points(bbox, corners, 8);
}


inline BoundingBox get_transformed_bbox(const BoundingBox* bbox, const glm::mat4& t)
{
	BoundingBox res = *bbox;
	transform_bbox(&res, t);
	return res;
}

inline void combine_bbox_point(BoundingBox* bbox, const glm::vec3& p)
{
	bbox->min = glm::min(bbox->min, p);
	bbox->max = glm::max(bbox->max, p);
}

inline void get_frustum_planes(glm::mat4 mvp, glm::vec4* planes)
{
	mvp = glm::transpose(mvp);
	planes[0] = glm::vec4(mvp[3] + mvp[0]); // left
	planes[1] = glm::vec4(mvp[3] - mvp[0]); // right
	planes[2] = glm::vec4(mvp[3] + mvp[1]); // bottom
	planes[3] = glm::vec4(mvp[3] - mvp[1]); // top
	planes[4] = glm::vec4(mvp[3] + mvp[2]); // near
	planes[5] = glm::vec4(mvp[3] - mvp[2]); // far
}

inline void get_frustum_corners(glm::mat4 mvp, glm::vec4* points)
{
	const glm::vec4 corners[] =
	{
		glm::vec4(-1, -1, -1, 1), glm::vec4(1, -1, -1, 1),
		glm::vec4(1,  1, -1, 1),  glm::vec4(-1,  1, -1, 1),
		glm::vec4(-1, -1,  1, 1), glm::vec4(1, -1,  1, 1),
		glm::vec4(1,  1,  1, 1),  glm::vec4(-1,  1,  1, 1)
	};
	
	const glm::mat4 invMVP = glm::inverse(mvp);

	for (int i = 0; i != 8; i++)
	{
		const glm::vec4 q = invMVP * corners[i];
		points[i] = q / q.w;
	}
}

inline bool is_box_in_frustum(glm::vec4* frustumPlanes, glm::vec4* frustumCorners, const BoundingBox* bbox)
{
	for (int i = 0; i < 6; i++) {
		int r = 0;
		r += (glm::dot(frustumPlanes[i], glm::vec4(bbox->min.x, bbox->min.y, bbox->min.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (glm::dot(frustumPlanes[i], glm::vec4(bbox->max.x, bbox->min.y, bbox->min.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (glm::dot(frustumPlanes[i], glm::vec4(bbox->min.x, bbox->max.y, bbox->min.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (glm::dot(frustumPlanes[i], glm::vec4(bbox->max.x, bbox->max.y, bbox->min.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (glm::dot(frustumPlanes[i], glm::vec4(bbox->min.x, bbox->min.y, bbox->max.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (glm::dot(frustumPlanes[i], glm::vec4(bbox->max.x, bbox->min.y, bbox->max.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (glm::dot(frustumPlanes[i], glm::vec4(bbox->min.x, bbox->max.y, bbox->max.z, 1.0f)) < 0.0) ? 1 : 0;
		r += (glm::dot(frustumPlanes[i], glm::vec4(bbox->max.x, bbox->max.y, bbox->max.z, 1.0f)) < 0.0) ? 1 : 0;
		if (r == 8) return false;
	}

	//check if frustum is inside/outside the box
	int r = 0;
	r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].x > bbox->max.x) ? 1 : 0); if (r == 8) return false;
	r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].x < bbox->min.x) ? 1 : 0); if (r == 8) return false;
	r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].y > bbox->max.y) ? 1 : 0); if (r == 8) return false;
	r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].y < bbox->min.y) ? 1 : 0); if (r == 8) return false;
	r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].z > bbox->max.z) ? 1 : 0); if (r == 8) return false;
	r = 0; for (int i = 0; i < 8; i++) r += ((frustumCorners[i].z < bbox->min.z) ? 1 : 0); if (r == 8) return false;

	return true;

}

inline BoundingBox combines_boxes(const std::vector<BoundingBox>& boxes)
{
	std::vector<glm::vec3> allPoints;
	allPoints.reserve(boxes.size() * 8);

	for (const auto& b : boxes)
	{
		allPoints.emplace_back(b.min.x, b.min.y, b.min.z);
		allPoints.emplace_back(b.min.x, b.min.y, b.max.z);
		allPoints.emplace_back(b.min.x, b.max.y, b.min.z);
		allPoints.emplace_back(b.min.x, b.max.y, b.max.z);

		allPoints.emplace_back(b.max.x, b.min.y, b.min.z);
		allPoints.emplace_back(b.max.x, b.min.y, b.max.z);
		allPoints.emplace_back(b.max.x, b.max.y, b.min.z);
		allPoints.emplace_back(b.max.x, b.max.y, b.max.z);
	}

	BoundingBox res;
	init_bbox_from_points(&res, allPoints.data(), allPoints.size());
	return res;
}