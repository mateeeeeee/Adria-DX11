#include "TextureManager.h"
#include <algorithm>
#include "ShaderCompiler.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "../Logging/Logger.h"
#include "../Utilities/StringUtil.h"
#include "../Core/Macros.h"
#include "../Utilities/Image.h"
#include "../Utilities/FilesUtil.h"

namespace adria
{
	namespace
	{
		
		enum class ETextureFormat
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

		ETextureFormat GetTextureFormat(std::string const& path)
		{
			std::string extension = GetExtension(path);
			std::transform(extension.begin(), extension.end(), extension.begin(),
				[](unsigned char c) { return std::tolower(c); });

			if (extension == ".dds")
				return ETextureFormat::DDS;
			else if (extension == ".bmp")
				return ETextureFormat::BMP;
			else if (extension == ".jpg" || extension == ".jpeg")
				return ETextureFormat::JPG;
			else if (extension == ".png")
				return ETextureFormat::PNG;
			else if (extension == ".tiff" || extension == ".tif")
				return ETextureFormat::TIFF;
			else if (extension == ".gif")
				return ETextureFormat::GIF;
			else if (extension == ".ico")
				return ETextureFormat::ICO;
			else if (extension == ".tga")
				return ETextureFormat::TGA;
			else if (extension == ".hdr")
				return ETextureFormat::HDR;
			else if (extension == ".pic")
				return ETextureFormat::PIC;
			else
				return ETextureFormat::NotSupported;
		}

		ETextureFormat GetTextureFormat(std::wstring const& path)
		{
			return GetTextureFormat(ConvertToNarrow(path));
		}

		constexpr UINT MipmapLevels(UINT width, UINT height)
		{
			UINT levels = 1U;
			while ((width | height) >> levels) ++levels;
			return levels;
		}

	}

TextureManager::TextureManager(ID3D11Device* _device, ID3D11DeviceContext* _context)
{
	ADRIA_ASSERT(_device != nullptr && _context != nullptr);

	device = _device;
	context = _context;

	mipmaps = true;
}

TextureHandle TextureManager::LoadTexture(std::wstring const& name)
{
	ETextureFormat format = GetTextureFormat(name);

	switch (format)
	{
	case ETextureFormat::DDS:
		return LoadDDSTexture(name);
	case ETextureFormat::BMP:
	case ETextureFormat::PNG:
	case ETextureFormat::JPG:
	case ETextureFormat::TIFF:
	case ETextureFormat::GIF:
	case ETextureFormat::ICO:
		return LoadWICTexture(name);
	case ETextureFormat::TGA:
	case ETextureFormat::HDR:
	case ETextureFormat::PIC:
		return LoadTexture_HDR_TGA_PIC(ConvertToNarrow(name));
	case ETextureFormat::NotSupported:
	default:
		ADRIA_ASSERT(false && "Unsupported Texture Format!");
	}

	return INVALID_TEXTURE_HANDLE;
}

TextureHandle TextureManager::LoadTexture(std::string const& name)
{
	return LoadTexture(ConvertToWide(name));
}

TextureHandle TextureManager::LoadCubeMap(std::wstring const& name)
{
	ETextureFormat format = GetTextureFormat(name);

	ADRIA_ASSERT(format == ETextureFormat::DDS || format == ETextureFormat::HDR && "Cubemap in one file has to be .dds or .hdr format");

	if (auto it = loaded_textures.find(name); it == loaded_textures.end())
	{
		++handle;
		if (format == ETextureFormat::DDS)
		{

			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubemap_srv;
			HRESULT hr = DirectX::CreateDDSTextureFromFileEx(device, name.c_str(), 0,
				D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, false, nullptr, &cubemap_srv);

			loaded_textures.insert({ name, handle });
			texture_map.insert({ handle, cubemap_srv });

		}
		else //HDR
		{
			Image equirect_hdr_image(ConvertToNarrow(name));

			Microsoft::WRL::ComPtr<ID3D11Texture2D> cubemap_tex = nullptr;
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

			BREAK_IF_FAILED(device->CreateTexture2D(&desc, nullptr, &cubemap_tex));

			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubemap_srv = nullptr;
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = desc.Format;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = -1;
			BREAK_IF_FAILED(device->CreateShaderResourceView(cubemap_tex.Get(), &srvDesc, &cubemap_srv));

			Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> cubemap_uav = nullptr;
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
			uavDesc.Format = desc.Format;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.MipSlice = 0;
			uavDesc.Texture2DArray.FirstArraySlice = 0;
			uavDesc.Texture2DArray.ArraySize = desc.ArraySize;

			BREAK_IF_FAILED(device->CreateUnorderedAccessView(cubemap_tex.Get(), &uavDesc, &cubemap_uav));
			Microsoft::WRL::ComPtr<ID3D11ComputeShader> equirect_to_cube{};

			ShaderBlob blob{};
			ShaderCompileInput input{};
			input.source_file = "Resources\\Shaders\\Deferred\\Equirect2cubeCS.hlsl";
			input.stage = EShaderStage::CS;
			input.entrypoint = "cs_main";
			ShaderCompileOutput output{};
			ShaderCompiler::CompileShader(input, output);
			device->CreateComputeShader(output.blob.GetPointer(), output.blob.GetLength(), nullptr, &equirect_to_cube);

			D3D11_SAMPLER_DESC samp_desc{};
			samp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			samp_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			samp_desc.MinLOD = 0;
			samp_desc.MaxLOD = D3D11_FLOAT32_MAX;
			Microsoft::WRL::ComPtr<ID3D11SamplerState> linear_wrap_sampler;
			BREAK_IF_FAILED(device->CreateSamplerState(&samp_desc, &linear_wrap_sampler));
			context->CSSetSamplers(0, 1, linear_wrap_sampler.GetAddressOf());

			Microsoft::WRL::ComPtr<ID3D11Texture2D> equirect_tex = nullptr;

			D3D11_TEXTURE2D_DESC equirect_desc = {};
			equirect_desc.Width = equirect_hdr_image.Width();
			equirect_desc.Height = equirect_hdr_image.Height();
			equirect_desc.MipLevels = 1;
			equirect_desc.ArraySize = 1;
			equirect_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			equirect_desc.SampleDesc.Count = 1;
			equirect_desc.Usage = D3D11_USAGE_DEFAULT;
			equirect_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

			BREAK_IF_FAILED(device->CreateTexture2D(&equirect_desc, nullptr, &equirect_tex));

			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> equirect_srv = nullptr;
			D3D11_SHADER_RESOURCE_VIEW_DESC equirect_srv_desc = {};
			equirect_srv_desc.Format = equirect_desc.Format;
			equirect_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			equirect_srv_desc.Texture2D.MostDetailedMip = 0;
			equirect_srv_desc.Texture2D.MipLevels = -1;
			BREAK_IF_FAILED(device->CreateShaderResourceView(equirect_tex.Get(), &equirect_srv_desc, &equirect_srv));


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
	ETextureFormat format = GetTextureFormat(cubemap_textures[0]);
	ADRIA_ASSERT(format == ETextureFormat::JPG || format == ETextureFormat::PNG || format == ETextureFormat::TGA ||
		format == ETextureFormat::BMP || format == ETextureFormat::HDR || format == ETextureFormat::PIC);

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

	for (UINT i = 0; i < cubemap_textures.size(); ++i)
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

	Microsoft::WRL::ComPtr<ID3D11Texture2D> tex_ptr;

	if (mipmaps)
	{
		BREAK_IF_FAILED(device->CreateTexture2D(&desc, nullptr, &tex_ptr));
		for (UINT i = 0; i < cubemap_textures.size(); ++i)
			context->UpdateSubresource(tex_ptr.Get(), D3D11CalcSubresource(0, i, desc.MipLevels), nullptr,
				subresource_data_array[i].pSysMem,
				subresource_data_array[i].SysMemPitch, 1);
	}
	else
	{
		BREAK_IF_FAILED(device->CreateTexture2D(&desc, subresource_data_array.data(), &tex_ptr));
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
	srv_desc.Format = desc.Format;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srv_desc.TextureCube.MostDetailedMip = 0;
	srv_desc.TextureCube.MipLevels = -1;

	ID3D11ShaderResourceView* pSRV = nullptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_ptr;
	HRESULT hr = device->CreateShaderResourceView(tex_ptr.Get(), &srv_desc, &view_ptr);
	BREAK_IF_FAILED(hr);

	if (mipmaps) context->GenerateMips(view_ptr.Get());
	texture_map.insert({ handle, view_ptr });
	return handle;
}

ID3D11ShaderResourceView* TextureManager::GetTextureView(TextureHandle tex_handle) const
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
	if (auto it = loaded_textures.find(name); it == loaded_textures.end())
	{
		++handle;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_ptr;
		ID3D11Texture2D* tex_ptr = nullptr;
		if (mipmaps)
		{
			HRESULT hr = DirectX::CreateDDSTextureFromFile(device, context, name.c_str(), reinterpret_cast<ID3D11Resource**>(&tex_ptr), view_ptr.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}
		else
		{
			HRESULT hr = DirectX::CreateDDSTextureFromFile(device, name.c_str(), reinterpret_cast<ID3D11Resource**>(&tex_ptr), view_ptr.GetAddressOf());
			BREAK_IF_FAILED(hr);
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
	if (auto it = loaded_textures.find(name); it == loaded_textures.end())
	{
		++handle;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_ptr;
		ID3D11Texture2D* tex_ptr = nullptr;
		if (mipmaps)
		{
			HRESULT hr = DirectX::CreateWICTextureFromFile(device, context, name.c_str(), reinterpret_cast<ID3D11Resource**>(&tex_ptr), view_ptr.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}
		else
		{
			HRESULT hr = DirectX::CreateWICTextureFromFile(device, name.c_str(), reinterpret_cast<ID3D11Resource**>(&tex_ptr), view_ptr.GetAddressOf());
			BREAK_IF_FAILED(hr);
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
	if (auto it = loaded_textures.find(name); it == loaded_textures.end())
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

		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex_ptr = nullptr;

		HRESULT hr = device->CreateTexture2D(&desc, nullptr, tex_ptr.GetAddressOf());
		BREAK_IF_FAILED(hr);

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
		srv_desc.Format = desc.Format;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MostDetailedMip = 0;
		srv_desc.Texture2D.MipLevels = -1;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_ptr = nullptr;
		hr = device->CreateShaderResourceView(tex_ptr.Get(), &srv_desc, view_ptr.GetAddressOf());
		BREAK_IF_FAILED(hr);
		context->UpdateSubresource(tex_ptr.Get(), 0, nullptr, img.Data<void>(), img.Pitch(), 0);
		if (mipmaps)
		{
			context->GenerateMips(view_ptr.Get());
		}
		loaded_textures.insert({ name, handle });
		texture_map.insert({ handle, view_ptr });

		return handle;
	}
	else return it->second;


}
}