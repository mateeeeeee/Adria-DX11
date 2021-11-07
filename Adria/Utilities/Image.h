#pragma once
#include <string_view>
#include <memory>
#include <vector>
#include "../Core/Definitions.h"

namespace adria
{
	class Image
	{
	public:
		Image(std::string_view image_file, i32 desired_channels = 4);

		Image(Image const&) = delete;
		Image(Image&&) = default;
		Image& operator=(Image const&) = delete;
		Image& operator=(Image&&) = default;
		~Image() = default;


		u32 Width() const;

		u32 Height() const;

		u32 Channels() const;

		u32 BytesPerPixel() const;

		u32 Pitch() const;

		bool IsHDR() const;

		template<typename T>
		T const* Data() const;

	private:
		u32 _width, _height;
		u32 _channels;
		std::unique_ptr<u8[]> _pixels;
		bool is_hdr;
	};

	template<typename T>
	T const* Image::Data() const
	{
		return reinterpret_cast<T const*>(_pixels.get());
	}

	void WriteImageTGA(char const* name, std::vector<u8> const& data, int width, int height);

	void WriteImagePNG(char const* name, std::vector<u8> const& data, int width, int height);

	void WriteImageJPG(char const* name, std::vector<u8> const& data, int width, int height);
}