#include "Image.h"
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include "../Logging/Logger.h"

namespace adria
{

	Image::Image(std::string_view image_file, i32 desired_channels /*= 4*/)
	{
		i32 width, height, channels;

		if (is_hdr = static_cast<bool>(stbi_is_hdr(image_file.data())); is_hdr)
		{
			f32* pixels = stbi_loadf(image_file.data(), &width, &height, &channels, desired_channels);
			if (!pixels) GLOBAL_LOG_ERROR("Loading Image File " + std::string(image_file.data()) + " Unsuccessful");
			else  _pixels.reset(reinterpret_cast<u8*>(pixels));
		}
		else
		{
			stbi_uc* pixels = stbi_load(image_file.data(), &width, &height, &channels, desired_channels);
			if (!pixels) GLOBAL_LOG_ERROR("Loading Image File " + std::string(image_file.data()) + " Unsuccessful");
			else _pixels.reset(pixels);
		}

		_width = static_cast<u32>(width);
		_height = static_cast<u32>(height);
		_channels = static_cast<u32>(desired_channels);
	}

	u32 Image::Width() const
	{
		return _width;
	}

	u32 Image::Height() const
	{
		return _height;
	}

	u32 Image::Channels() const
	{
		return _channels;
	}

	u32 Image::BytesPerPixel() const
	{
		return _channels * (is_hdr ? sizeof(f32) : sizeof(u8));
	}

	u32 Image::Pitch() const
	{
		return _width * BytesPerPixel();
	}

	bool Image::IsHDR() const
	{
		return is_hdr;
	}

	void WriteImageTGA(char const* name, std::vector<stbi_uc> const& data, int width, int height)
	{
		stbi_write_tga(name, width, height, STBI_rgb_alpha, (void*)data.data());
	}

	void WriteImagePNG(char const* name, std::vector<stbi_uc> const& data, int width, int height)
	{
		stbi_write_png(name, width, height, STBI_rgb_alpha, (void*)data.data(), width * 4 * sizeof(stbi_uc));
	}

}

