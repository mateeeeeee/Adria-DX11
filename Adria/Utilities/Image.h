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
		Image(std::string_view image_file, I32 desired_channels = 4);

		Image(Image const&) = delete;
		Image(Image&&) = default;
		Image& operator=(Image const&) = delete;
		Image& operator=(Image&&) = default;
		~Image() = default;


		U32 Width() const;

		U32 Height() const;

		U32 Channels() const;

		U32 BytesPerPixel() const;

		U32 Pitch() const;

		bool IsHDR() const;

		template<typename T>
		T const* Data() const;

	private:
		U32 _width, _height;
		U32 _channels;
		std::unique_ptr<U8[]> _pixels;
		bool is_hdr;
	};

	template<typename T>
	T const* Image::Data() const
	{
		return reinterpret_cast<T const*>(_pixels.get());
	}

	void WriteImageTGA(char const* name, std::vector<U8> const& data, int width, int height);

	void WriteImagePNG(char const* name, std::vector<U8> const& data, int width, int height);

	void WriteImageJPG(char const* name, std::vector<U8> const& data, int width, int height);
}