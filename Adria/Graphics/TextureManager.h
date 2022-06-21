#pragma once
#include <d3d11.h>
#include <string>
#include <array>
#include <wrl.h>
#include <unordered_map>
#include <variant>


namespace adria
{
	using TextureHandle = size_t;

	inline constexpr TextureHandle const INVALID_TEXTURE_HANDLE = size_t(-1);

	class TextureManager
	{
	public:
		TextureManager(ID3D11Device* _device, ID3D11DeviceContext* _context);
		TextureManager(TextureManager const&) = delete;
		TextureManager& operator=(TextureManager const&) = delete;
		~TextureManager() = default;

		[[nodiscard]] TextureHandle LoadTexture(std::wstring const& name);

		[[nodiscard]] TextureHandle LoadTexture(std::string const& name);

		[[nodiscard]] TextureHandle LoadCubeMap(std::wstring const& name);

		[[nodiscard]] TextureHandle LoadCubeMap(std::array<std::string, 6> const& cubemap_textures);

		ID3D11ShaderResourceView* GetTextureView(TextureHandle tex_handle) const;

		void SetMipMaps(bool mipmaps);

	private:

		bool mipmaps = true;
		TextureHandle handle = INVALID_TEXTURE_HANDLE;
		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;
		std::unordered_map<TextureHandle, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> texture_map{};
		std::unordered_map<std::variant<std::wstring, std::string>, TextureHandle> loaded_textures{};

	private:

		TextureHandle LoadDDSTexture(std::wstring const& name);

		TextureHandle LoadWICTexture(std::wstring const& name);

		TextureHandle LoadTexture_HDR_TGA_PIC(std::string const& name);
	};

}