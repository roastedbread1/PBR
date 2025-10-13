#include <bitmap.h>
#include <string.h>
int bitmap_get_bytes_per_component(eBitmapFormat fmt)
{
	if (fmt == eBitmapFormat_UnsignedByte) return 1;
	if (fmt == eBitmapFormat_Float) return 4;
	return 0;
}

void init_bitmap(Bitmap* bitmap, int w, int h, int comp, eBitmapFormat fmt)
{
	bitmap->w = w;
	bitmap->h = h;
	bitmap->d = 1;
	bitmap->comp = comp;
	bitmap->fmt = fmt;
	bitmap->data.resize(w * h * comp * bitmap_get_bytes_per_component(fmt));
}

void init_bitmap_3d(Bitmap* bitmap, int w, int h, int d, int comp, eBitmapFormat fmt)
{
	bitmap->w = w;
	bitmap->h = h;
	bitmap->d = d;
	bitmap->comp = comp;
	bitmap->fmt = fmt;
	bitmap->data.resize(w * h * d * comp * bitmap_get_bytes_per_component(fmt));
}


void init_bitmap_from_data(Bitmap* bitmap, int w, int h, int comp, eBitmapFormat fmt, const void* ptr)
{
	init_bitmap(bitmap, w, h, comp, fmt);
	memcpy(bitmap->data.data(), ptr, bitmap->data.size());
}

void set_bitmap_pixel(Bitmap* bitmap, int x, int y, const glm::vec4& c)
{
	switch (bitmap->fmt)
	{
	case eBitmapFormat_UnsignedByte:
		return set_bitmap_pixel_unsigned_byte(bitmap, x, y, c);
		break;
	case eBitmapFormat_Float:
		return set_bitmap_pixel_float(bitmap, x, y, c);
		break;
	}
}

glm::vec4 get_bitmap_pixel(const Bitmap* bitmap, int x, int y)
{
	switch (bitmap->fmt)
	{
	case eBitmapFormat_UnsignedByte:
		return get_bitmap_pixel_unsigned_byte(bitmap, x, y);
		break;
	case eBitmapFormat_Float:
		return get_bitmap_pixel_float(bitmap, x, y);
		break;
	}
	return glm::vec4(0xCD);
}


static void set_bitmap_pixel_float(Bitmap* bitmap, int x, int y, const glm::vec4& c)
{
	const int ofs = bitmap->comp * (y * bitmap->w + x);
	float* data = reinterpret_cast<float*>(bitmap->data.data());
	if (bitmap->comp > 0) data[ofs + 0] = c.x;
	if (bitmap->comp > 1) data[ofs + 1] = c.y;
	if (bitmap->comp > 2) data[ofs + 2] = c.z;
	if (bitmap->comp > 3) data[ofs + 3] = c.w;
}

void set_bitmap_pixel_unsigned_byte(Bitmap* bitmap, int x, int y, const glm::vec4& c)
{
	const int ofs = bitmap->comp * (y * bitmap->w + x);
	if (bitmap->comp > 0)  bitmap->data[ofs + 0] = uint8_t(c.x * 255.0f);
	if (bitmap->comp > 1)  bitmap->data[ofs + 1] = uint8_t(c.y * 255.0f);
	if (bitmap->comp > 2)  bitmap->data[ofs + 2] = uint8_t(c.z * 255.0f);
	if (bitmap->comp > 3)  bitmap->data[ofs + 3] = uint8_t(c.w * 255.0f);
}

glm::vec4 get_bitmap_pixel_float(const Bitmap* bitmap, int x, int y)
{
	const int ofs = bitmap->comp * (y * bitmap->w + x);
	const float* data = reinterpret_cast<const float*>(bitmap->data.data());
	return glm::vec4(
		bitmap->comp > 0 ? data[ofs + 0] : 0.0f,
		bitmap->comp > 1 ? data[ofs + 1] : 0.0f,
		bitmap->comp > 2 ? data[ofs + 2] : 0.0f,
		bitmap->comp > 3 ? data[ofs + 3] : 0.0f);
}


glm::vec4 get_bitmap_pixel_unsigned_byte(const Bitmap* bitmap, int x, int y)
{
	const int ofs = bitmap->comp * (y * bitmap->w + x);
	return glm::vec4(
		bitmap->comp > 0 ? float(bitmap->data[ofs + 0]) / 255.0f : 0.0f,
		bitmap->comp > 1 ? float(bitmap->data[ofs + 1]) / 255.0f : 0.0f,
		bitmap->comp > 2 ? float(bitmap->data[ofs + 2]) / 255.0f : 0.0f,
		bitmap->comp > 3 ? float(bitmap->data[ofs + 3]) / 255.0f : 0.0f
	);
}