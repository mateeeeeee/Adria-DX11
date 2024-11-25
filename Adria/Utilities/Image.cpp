#include "Image.h"
#define STB_IMAGE_IMPLEMENTATION  
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include "Core/Logger.h"

namespace adria
{

	Image::Image(std::string_view image_file, Int32 desired_channels /*= 4*/)
	{
		Int32 width, height, channels;

		if (is_hdr = static_cast<Bool>(stbi_is_hdr(image_file.data())); is_hdr)
		{
			Float* pixels = stbi_loadf(image_file.data(), &width, &height, &channels, desired_channels);
			if (!pixels) 
			{ 
				ADRIA_LOG(ERROR, "Loading image file %s unsuccessful", image_file.data()); 
			}
			else  _pixels.reset(reinterpret_cast<Uint8*>(pixels));
		}
		else
		{
			stbi_uc* pixels = stbi_load(image_file.data(), &width, &height, &channels, desired_channels);
			if (!pixels)
			{
				ADRIA_LOG(ERROR, "Loading image file %s unsuccessful", image_file.data());
			}
			else _pixels.reset(pixels);
		}

		_width = static_cast<Uint32>(width);
		_height = static_cast<Uint32>(height);
		_channels = static_cast<Uint32>(desired_channels);
	}

	Uint32 Image::Width() const
	{
		return _width;
	}

	Uint32 Image::Height() const
	{
		return _height;
	}

	Uint32 Image::Channels() const
	{
		return _channels;
	}

	Uint32 Image::BytesPerPixel() const
	{
		return _channels * (is_hdr ? sizeof(Float) : sizeof(Uint8));
	}

	Uint32 Image::Pitch() const
	{
		return _width * BytesPerPixel();
	}

	Bool Image::IsHDR() const
	{
		return is_hdr;
	}

	void WriteImageTGA(Char const* name, std::vector<Uint8> const& data, int width, int height)
	{
		stbi_write_tga(name, width, height, STBI_rgb_alpha, (void*)data.data());
	}

	void WriteImagePNG(Char const* name, std::vector<Uint8> const& data, int width, int height)
	{
		stbi_write_png(name, width, height, STBI_rgb_alpha, (void*)data.data(), width * 4 * sizeof(stbi_uc));
	}

	void WriteImageJPG(Char const* name, std::vector<Uint8> const& data, int width, int height)
	{
		stbi_write_jpg(name, width, height, STBI_rgb_alpha, (void*)data.data(), 100);
	}

}

