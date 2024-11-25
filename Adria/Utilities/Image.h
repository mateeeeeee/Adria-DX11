#pragma once
#include <string_view>
#include <memory>
#include <vector>

namespace adria
{
	class Image
	{
	public:
		Image(std::string_view image_file, Int32 desired_channels = 4);

		Image(Image const&) = delete;
		Image(Image&&) = default;
		Image& operator=(Image const&) = delete;
		Image& operator=(Image&&) = default;
		~Image() = default;


		Uint32 Width() const;

		Uint32 Height() const;

		Uint32 Channels() const;

		Uint32 BytesPerPixel() const;

		Uint32 Pitch() const;

		Bool IsHDR() const;

		template<typename T>
		T const* Data() const;

	private:
		Uint32 _width, _height;
		Uint32 _channels;
		std::unique_ptr<Uint8[]> _pixels;
		Bool is_hdr;
	};

	template<typename T>
	T const* Image::Data() const
	{
		return reinterpret_cast<T const*>(_pixels.get());
	}

	void WriteImageTGA(Char const* name, std::vector<Uint8> const& data, int width, int height);
	void WriteImagePNG(Char const* name, std::vector<Uint8> const& data, int width, int height);
	void WriteImageJPG(Char const* name, std::vector<Uint8> const& data, int width, int height);
}