#pragma once
#include <d3d11.h>
#include <string>
#include <array>
#include <wrl.h>
#include <unordered_map>
#include <variant>

namespace adria
{
	using TEXTURE_HANDLE = size_t;

	inline constexpr TEXTURE_HANDLE const INVALID_TEXTURE_HANDLE = size_t(-1);

	class TextureManager
	{
	public:
		TextureManager(ID3D11Device* _device, ID3D11DeviceContext* _context);
		TextureManager(TextureManager const&) = delete;
		TextureManager& operator=(TextureManager const&) = delete;
		~TextureManager() = default;

		[[nodiscard]] TEXTURE_HANDLE LoadTexture(std::wstring const& name);

		[[nodiscard]] TEXTURE_HANDLE LoadTexture(std::string const& name);

		[[nodiscard]] TEXTURE_HANDLE LoadCubeMap(std::wstring const& name);

		[[nodiscard]] TEXTURE_HANDLE LoadCubeMap(std::array<std::string, 6> const& cubemap_textures);

		ID3D11ShaderResourceView* GetTextureView(TEXTURE_HANDLE tex_handle) const;

		void SetMipMaps(bool mipmaps);

	private:

		bool mipmaps = true;
		TEXTURE_HANDLE handle = INVALID_TEXTURE_HANDLE;
		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;
		std::unordered_map<TEXTURE_HANDLE, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> texture_map{};
		std::unordered_map<std::variant<std::wstring, std::string>, TEXTURE_HANDLE> loaded_textures{};

	private:

		TEXTURE_HANDLE LoadDDSTexture(std::wstring const& name);

		TEXTURE_HANDLE LoadWICTexture(std::wstring const& name);

		TEXTURE_HANDLE LoadTexture_HDR_TGA_PIC(std::string const& name);
	};

}