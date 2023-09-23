#pragma once
#include <string>
#include <array>
#include <unordered_map>
#include "Graphics/GfxDevice.h"
#include "Utilities/Singleton.h"

namespace adria
{
	using TextureHandle = size_t;
	inline constexpr TextureHandle const INVALID_TEXTURE_HANDLE = size_t(-1);

	class TextureManager : public Singleton<TextureManager>
	{
		friend class Singleton<TextureManager>;

	public:
		void Initialize(GfxDevice* gfx);
		void Destroy();

		[[nodiscard]] TextureHandle LoadTexture(std::wstring const& name);
		[[nodiscard]] TextureHandle LoadTexture(std::string const& name);
		[[nodiscard]] TextureHandle LoadCubeMap(std::wstring const& name);
		[[nodiscard]] TextureHandle LoadCubeMap(std::array<std::string, 6> const& cubemap_textures);

		ID3D11ShaderResourceView* GetTextureView(TextureHandle tex_handle) const;
		void SetMipMaps(bool mipmaps);

	private:
		GfxDevice* gfx;
		bool mipmaps = true;
		TextureHandle handle = INVALID_TEXTURE_HANDLE;
		std::unordered_map<TextureHandle, ArcPtr<ID3D11ShaderResourceView>> texture_map{};
		std::unordered_map<std::wstring, TextureHandle> loaded_textures{};

	private:
		TextureManager() = default;
		TextureManager(TextureManager const&) = delete;
		TextureManager& operator=(TextureManager const&) = delete;
		~TextureManager() = default;

		TextureHandle LoadDDSTexture(std::wstring const& name);
		TextureHandle LoadWICTexture(std::wstring const& name);
		TextureHandle LoadTexture_HDR_TGA_PIC(std::string const& name);
	};
	#define g_TextureManager TextureManager::Get()
}