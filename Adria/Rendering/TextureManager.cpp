#include <algorithm>
#include "TextureManager.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "Graphics/GfxShaderCompiler.h"
#include "Utilities/StringUtil.h"
#include "Utilities/Image.h"
#include "Utilities/FilesUtil.h"

using namespace DirectX;

namespace adria
{
	namespace
	{
		enum class TextureFormat
		{
			DDS,
			BMP,
			JPG,
			PNG,
			TIFF,
			GIF,
			ICO,
			TGA,
			HDR,
			PIC,
			NotSupported
		};
		TextureFormat GetTextureFormat(std::string const& path)
		{
			std::string extension = GetExtension(path);
			std::transform(extension.begin(), extension.end(), extension.begin(),
				[](unsigned char c) { return std::tolower(c); });

			if (extension == ".dds")
				return TextureFormat::DDS;
			else if (extension == ".bmp")
				return TextureFormat::BMP;
			else if (extension == ".jpg" || extension == ".jpeg")
				return TextureFormat::JPG;
			else if (extension == ".png")
				return TextureFormat::PNG;
			else if (extension == ".tiff" || extension == ".tif")
				return TextureFormat::TIFF;
			else if (extension == ".gif")
				return TextureFormat::GIF;
			else if (extension == ".ico")
				return TextureFormat::ICO;
			else if (extension == ".tga")
				return TextureFormat::TGA;
			else if (extension == ".hdr")
				return TextureFormat::HDR;
			else if (extension == ".pic")
				return TextureFormat::PIC;
			else
				return TextureFormat::NotSupported;
		}
		TextureFormat GetTextureFormat(std::wstring const& path)
		{
			return GetTextureFormat(ToString(path));
		}
		constexpr uint32 MipmapLevels(uint32 width, uint32 height)
		{
			uint32 levels = 1U;
			while ((width | height) >> levels) ++levels;
			return levels;
		}
	}


void TextureManager::Initialize(GfxDevice* _gfx)
{
	gfx = _gfx;
	mipmaps = true;
}

void TextureManager::Destroy()
{
	gfx = nullptr;
}

TextureHandle TextureManager::LoadTexture(std::wstring const& name)
{
	TextureFormat format = GetTextureFormat(name);

	switch (format)
	{
	case TextureFormat::DDS:
		return LoadDDSTexture(name);
	case TextureFormat::BMP:
	case TextureFormat::PNG:
	case TextureFormat::JPG:
	case TextureFormat::TIFF:
	case TextureFormat::GIF:
	case TextureFormat::ICO:
		return LoadWICTexture(name);
	case TextureFormat::TGA:
	case TextureFormat::HDR:
	case TextureFormat::PIC:
		return LoadTexture_HDR_TGA_PIC(ToString(name));
	case TextureFormat::NotSupported:
	default:
		ADRIA_ASSERT(false && "Unsupported Texture Format!");
	}

	return INVALID_TEXTURE_HANDLE;
}

TextureHandle TextureManager::LoadTexture(std::string const& name)
{
	return LoadTexture(ToWideString(name));
}

TextureHandle TextureManager::LoadCubeMap(std::wstring const& name)
{
	TextureFormat format = GetTextureFormat(name);
	ADRIA_ASSERT(format == TextureFormat::DDS || format == TextureFormat::HDR && "Cubemap in one file has to be .dds or .hdr format");

	ID3D11Device* device = gfx->GetDevice();
	ID3D11DeviceContext* context = gfx->GetContext();
	if (auto it = loaded_textures.find(name); it == loaded_textures.end())
	{
		++handle;
		if (format == TextureFormat::DDS)
		{

			ArcPtr<ID3D11ShaderResourceView> cubemap_srv;
			HRESULT hr = CreateDDSTextureFromFileEx(device, name.c_str(), 0,
				D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, false, nullptr, cubemap_srv.GetAddressOf());

			loaded_textures.insert({ name, handle });
			texture_map.insert({ handle, cubemap_srv });

		}
		else //HDR
		{
			Image equirect_hdr_image(ToString(name));
			ArcPtr<ID3D11Texture2D> cubemap_tex = nullptr;
			D3D11_TEXTURE2D_DESC desc{};
			desc.Width = 1024;
			desc.Height = 1024;
			desc.MipLevels = 0;
			desc.ArraySize = 6;
			desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			desc.SampleDesc.Count = 1;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
			desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

			GFX_CHECK_HR(device->CreateTexture2D(&desc, nullptr, cubemap_tex.GetAddressOf()));

			ArcPtr<ID3D11ShaderResourceView> cubemap_srv = nullptr;
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = desc.Format;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = -1;
			GFX_CHECK_HR(device->CreateShaderResourceView(cubemap_tex.Get(), &srvDesc, cubemap_srv.GetAddressOf()));

			ArcPtr<ID3D11UnorderedAccessView> cubemap_uav = nullptr;
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
			uavDesc.Format = desc.Format;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.MipSlice = 0;
			uavDesc.Texture2DArray.FirstArraySlice = 0;
			uavDesc.Texture2DArray.ArraySize = desc.ArraySize;

			GFX_CHECK_HR(device->CreateUnorderedAccessView(cubemap_tex.Get(), &uavDesc, cubemap_uav.GetAddressOf()));
			ArcPtr<ID3D11ComputeShader> equirect_to_cube{};

			GfxShaderBlob blob{};
			GfxShaderDesc input{};
			input.source_file = "Resources\\Shaders\\Deferred\\Equirect2cubeCS.hlsl";
			input.stage = GfxShaderStage::CS;
			input.entrypoint = "cs_main";
			GfxShaderCompileOutput output{};
			GfxShaderCompiler::CompileShader(input, output);
			device->CreateComputeShader(output.shader_bytecode.GetPointer(), output.shader_bytecode.GetLength(), nullptr, equirect_to_cube.GetAddressOf());

			D3D11_SAMPLER_DESC samp_desc{};
			samp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			samp_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			samp_desc.MinLOD = 0;
			samp_desc.MaxLOD = D3D11_FLOAT32_MAX;
			ArcPtr<ID3D11SamplerState> linear_wrap_sampler;
			GFX_CHECK_HR(device->CreateSamplerState(&samp_desc, linear_wrap_sampler.GetAddressOf()));
			context->CSSetSamplers(0, 1, linear_wrap_sampler.GetAddressOf());

			ArcPtr<ID3D11Texture2D> equirect_tex = nullptr;

			D3D11_TEXTURE2D_DESC equirect_desc = {};
			equirect_desc.Width = equirect_hdr_image.Width();
			equirect_desc.Height = equirect_hdr_image.Height();
			equirect_desc.MipLevels = 1;
			equirect_desc.ArraySize = 1;
			equirect_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			equirect_desc.SampleDesc.Count = 1;
			equirect_desc.Usage = D3D11_USAGE_DEFAULT;
			equirect_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

			GFX_CHECK_HR(device->CreateTexture2D(&equirect_desc, nullptr, equirect_tex.GetAddressOf()));

			ArcPtr<ID3D11ShaderResourceView> equirect_srv = nullptr;
			D3D11_SHADER_RESOURCE_VIEW_DESC equirect_srv_desc = {};
			equirect_srv_desc.Format = equirect_desc.Format;
			equirect_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			equirect_srv_desc.Texture2D.MostDetailedMip = 0;
			equirect_srv_desc.Texture2D.MipLevels = -1;
			GFX_CHECK_HR(device->CreateShaderResourceView(equirect_tex.Get(), &equirect_srv_desc, equirect_srv.GetAddressOf()));

			context->UpdateSubresource(equirect_tex.Get(), 0, nullptr, equirect_hdr_image.Data<void>(), equirect_hdr_image.Pitch(), 0);
			context->CSSetShaderResources(0, 1, equirect_srv.GetAddressOf());
			context->CSSetUnorderedAccessViews(0, 1, cubemap_uav.GetAddressOf(), nullptr);
			context->CSSetShader(equirect_to_cube.Get(), nullptr, 0);
			context->Dispatch(desc.Width / 32, desc.Height / 32, 6);

			ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
			context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);

			context->GenerateMips(cubemap_srv.Get());

			loaded_textures.insert({ name, handle });
			texture_map.insert({ handle, cubemap_srv });

		}
		return handle;
	}
	else return it->second;
}

TextureHandle TextureManager::LoadCubeMap(std::array<std::string, 6> const& cubemap_textures)
{
	TextureFormat format = GetTextureFormat(cubemap_textures[0]);
	ADRIA_ASSERT(format == TextureFormat::JPG || format == TextureFormat::PNG || format == TextureFormat::TGA ||
		format == TextureFormat::BMP || format == TextureFormat::HDR || format == TextureFormat::PIC);
	
	ID3D11Device* device = gfx->GetDevice();
	ID3D11DeviceContext* context = gfx->GetContext();

	++handle;
	D3D11_TEXTURE2D_DESC desc{};
	desc.ArraySize = 6;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = mipmaps ? D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET : D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = mipmaps ? D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS : D3D11_RESOURCE_MISC_TEXTURECUBE;

	std::vector<Image> images{};
	std::vector<D3D11_SUBRESOURCE_DATA> subresource_data_array;
	for (uint32 i = 0; i < cubemap_textures.size(); ++i)
	{
		images.emplace_back(cubemap_textures[i], 4);

		D3D11_SUBRESOURCE_DATA subresource_data{};
		subresource_data.pSysMem = images.back().Data<void>();
		subresource_data.SysMemPitch = images.back().Pitch();
		subresource_data_array.push_back(subresource_data);
	}

	desc.Width = images[0].Width();
	desc.Height = images[0].Height();
	desc.Format = images[0].IsHDR() ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.MipLevels = mipmaps ? MipmapLevels(desc.Width, desc.Height) : 1;

	ArcPtr<ID3D11Texture2D> tex_ptr = nullptr;
	if (mipmaps)
	{
		GFX_CHECK_HR(device->CreateTexture2D(&desc, nullptr, tex_ptr.GetAddressOf()));
		for (uint32 i = 0; i < cubemap_textures.size(); ++i)
			context->UpdateSubresource(tex_ptr.Get(), D3D11CalcSubresource(0, i, desc.MipLevels), nullptr,
								subresource_data_array[i].pSysMem, subresource_data_array[i].SysMemPitch, 1);
	}
	else
	{
		GFX_CHECK_HR(device->CreateTexture2D(&desc, subresource_data_array.data(), tex_ptr.GetAddressOf()));
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
	srv_desc.Format = desc.Format;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srv_desc.TextureCube.MostDetailedMip = 0;
	srv_desc.TextureCube.MipLevels = -1;

	ArcPtr<ID3D11ShaderResourceView> view_ptr;
	HRESULT hr = device->CreateShaderResourceView(tex_ptr.Get(), &srv_desc, view_ptr.GetAddressOf());
	GFX_CHECK_HR(hr);

	if (mipmaps) context->GenerateMips(view_ptr.Get());
	texture_map.insert({ handle, view_ptr });
	return handle;
}

GfxShaderResourceRO TextureManager::GetTextureDescriptor(TextureHandle tex_handle) const
{
	if (auto it = texture_map.find(tex_handle); it != texture_map.end()) return it->second.Get();
	else return nullptr;
}

void TextureManager::SetMipMaps(bool _mipmaps)
{
	mipmaps = _mipmaps;
}

TextureHandle TextureManager::LoadDDSTexture(std::wstring const& name)
{
	ID3D11Device* device = gfx->GetDevice();
	ID3D11DeviceContext* context = gfx->GetContext();
	if (auto it = loaded_textures.find(name); it == loaded_textures.end())
	{
		++handle;
		GfxArcShaderResourceRO view_ptr;
		ID3D11Texture2D* tex_ptr = nullptr;
		if (mipmaps)
		{
			HRESULT hr = CreateDDSTextureFromFile(device, context, name.c_str(), reinterpret_cast<ID3D11Resource**>(&tex_ptr), view_ptr.GetAddressOf());
			GFX_CHECK_HR(hr);
		}
		else
		{
			HRESULT hr = CreateDDSTextureFromFile(device, name.c_str(), reinterpret_cast<ID3D11Resource**>(&tex_ptr), view_ptr.GetAddressOf());
			GFX_CHECK_HR(hr);
		}

		loaded_textures.insert({ name, handle });
		texture_map.insert({ handle, view_ptr });
		tex_ptr->Release();
		return handle;
	}
	else return it->second;
}

TextureHandle TextureManager::LoadWICTexture(std::wstring const& name)
{
	ID3D11Device* device = gfx->GetDevice();
	ID3D11DeviceContext* context = gfx->GetContext();

	if (auto it = loaded_textures.find(name); it == loaded_textures.end())
	{
		++handle;

		ArcPtr<ID3D11ShaderResourceView> view_ptr;
		ID3D11Texture2D* tex_ptr = nullptr;
		if (mipmaps)
		{
			HRESULT hr = CreateWICTextureFromFile(device, context, name.c_str(), reinterpret_cast<ID3D11Resource**>(&tex_ptr), view_ptr.GetAddressOf());
			GFX_CHECK_HR(hr);
		}
		else
		{
			HRESULT hr = CreateWICTextureFromFile(device, name.c_str(), reinterpret_cast<ID3D11Resource**>(&tex_ptr), view_ptr.GetAddressOf());
			GFX_CHECK_HR(hr);
		}

		loaded_textures.insert({ name, handle });
		texture_map.insert({ handle, view_ptr });
		tex_ptr->Release();
		return handle;
	}
	else return it->second;
}

TextureHandle TextureManager::LoadTexture_HDR_TGA_PIC(std::string const& name)
{
	ID3D11Device* device = gfx->GetDevice();
	ID3D11DeviceContext* context = gfx->GetContext();

	std::wstring wide_name = ToWideString(name);
	if (auto it = loaded_textures.find(wide_name); it == loaded_textures.end())
	{
		++handle;
		Image img(name, 4);
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = img.Width();
		desc.Height = img.Height();
		desc.MipLevels = mipmaps ? 0 : 1;
		desc.ArraySize = 1;
		desc.Format = img.IsHDR() ? DXGI_FORMAT_R32G32B32A32_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		if (mipmaps)
		{
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}

		ArcPtr<ID3D11Texture2D> tex_ptr = nullptr;

		HRESULT hr = device->CreateTexture2D(&desc, nullptr, tex_ptr.GetAddressOf());
		GFX_CHECK_HR(hr);

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
		srv_desc.Format = desc.Format;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MostDetailedMip = 0;
		srv_desc.Texture2D.MipLevels = -1;

		ArcPtr<ID3D11ShaderResourceView> view_ptr = nullptr;
		hr = device->CreateShaderResourceView(tex_ptr.Get(), &srv_desc, view_ptr.GetAddressOf());
		GFX_CHECK_HR(hr);
		context->UpdateSubresource(tex_ptr.Get(), 0, nullptr, img.Data<void>(), img.Pitch(), 0);
		if (mipmaps)
		{
			context->GenerateMips(view_ptr.Get());
		}
		loaded_textures.insert({ wide_name, handle });
		texture_map.insert({ handle, view_ptr });

		return handle;
	}
	else return it->second;


}
}