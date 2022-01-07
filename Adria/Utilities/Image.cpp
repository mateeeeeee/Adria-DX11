#include "Image.h"
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include "../Logging/Logger.h"

namespace adria
{

	Image::Image(std::string_view image_file, I32 desired_channels /*= 4*/)
	{
		I32 width, height, channels;

		if (is_hdr = static_cast<bool>(stbi_is_hdr(image_file.data())); is_hdr)
		{
			F32* pixels = stbi_loadf(image_file.data(), &width, &height, &channels, desired_channels);
			if (!pixels) 
			{ 
				ADRIA_LOG(ERROR, "Loading Image File %s unsuccessful", image_file.data()); 
			}
			else  _pixels.reset(reinterpret_cast<U8*>(pixels));
		}
		else
		{
			stbi_uc* pixels = stbi_load(image_file.data(), &width, &height, &channels, desired_channels);
			if (!pixels)
			{
				ADRIA_LOG(ERROR, "Loading Image File %s unsuccessful", image_file.data());
			}
			else _pixels.reset(pixels);
		}

		_width = static_cast<U32>(width);
		_height = static_cast<U32>(height);
		_channels = static_cast<U32>(desired_channels);
	}

	U32 Image::Width() const
	{
		return _width;
	}

	U32 Image::Height() const
	{
		return _height;
	}

	U32 Image::Channels() const
	{
		return _channels;
	}

	U32 Image::BytesPerPixel() const
	{
		return _channels * (is_hdr ? sizeof(F32) : sizeof(U8));
	}

	U32 Image::Pitch() const
	{
		return _width * BytesPerPixel();
	}

	bool Image::IsHDR() const
	{
		return is_hdr;
	}

	void WriteImageTGA(char const* name, std::vector<U8> const& data, int width, int height)
	{
		stbi_write_tga(name, width, height, STBI_rgb_alpha, (void*)data.data());
	}

	void WriteImagePNG(char const* name, std::vector<U8> const& data, int width, int height)
	{
		stbi_write_png(name, width, height, STBI_rgb_alpha, (void*)data.data(), width * 4 * sizeof(stbi_uc));
	}

	void WriteImageJPG(char const* name, std::vector<U8> const& data, int width, int height)
	{
		stbi_write_jpg(name, width, height, STBI_rgb_alpha, (void*)data.data(), 100);
	}

}

