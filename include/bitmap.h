#pragma once
#include <string.h>
#include <vector>
#include <glm/glm.hpp>

enum eBitmapType
{
	eBitmapType_2D,
	eBitmapType_Cube
};

enum eBitmapFormat
{
	eBitmapFormat_UnsignedByte,
	eBitmapFormat_Float
};

struct Bitmap
{
	int w = 0xCD;
	int h = 0xCD;
	int d = 1;
	int comp = 3;
	eBitmapFormat fmt = eBitmapFormat_UnsignedByte;
	eBitmapType type = eBitmapType_2D;
	std::vector<uint8_t> data;
};

int bitmap_get_bytes_per_component(eBitmapFormat fmt);
void init_bitmap(Bitmap* bitmap, int w, int h, int comp, eBitmapFormat fmt);
void init_bitmap_3d(Bitmap* bitmap, int w, int h, int d, int comp, eBitmapFormat fmt);
void init_bitmap_from_data(Bitmap* bitmap, int w, int h, int comp, eBitmapFormat fmt, const void* ptr);
void set_bitmap_pixel(Bitmap* bitmap, int x, int y, const glm::vec4& c);
glm::vec4 get_bitmap_pixel(const Bitmap* bitmap, int x, int y);


void set_bitmap_pixel_float(Bitmap* bitmap, int x, int y, const glm::vec4& c);
void set_bitmap_pixel_unsigned_byte(Bitmap* bitmap, int x, int y, const glm::vec4& c);
glm::vec4 get_bitmap_pixel_float(const Bitmap* bitmap, int x, int y);
glm::vec4 get_bitmap_pixel_unsigned_byte(const Bitmap* bitmap, int x, int y);