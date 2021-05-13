#include "TextureManager.h"
#include <algorithm>
#include "ShaderUtility.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "../Logging/Logger.h"
#include "../Utilities/StringUtil.h"
#include "../Core/Macros.h"
#include "../Utilities/Image.h"

namespace adria
{
	namespace
	{
		
		enum class TextureFormat
		{
			eDDS,
			eBMP,
			eJPG,
			ePNG,
			eTIFF,
			eGIF,
			eICO,
			eTGA,
			eHDR,
			ePIC,
			eNotSupported
		};

		TextureFormat GetTextureFormat(std::string const& path)
		{
			std::string extension = path.substr(path.find_last_of(".") + 1);

			std::transform(extension.begin(), extension.end(), extension.begin(),
				[](unsigned char c) { return std::tolower(c); });

			if (extension == "dds")
				return TextureFormat::eDDS;
			else if (extension == "bmp")
				return TextureFormat::eBMP;
			else if (extension == "jpg" || extension == "jpeg")
				return TextureFormat::eJPG;
			else if (extension == "png")
				return TextureFormat::ePNG;
			else if (extension == "tiff" || extension == "tif")
				return TextureFormat::eTIFF;
			else if (extension == "gif")
				return TextureFormat::eGIF;
			else if (extension == "ico")
				return TextureFormat::eICO;
			else if (extension == "tga")
				return TextureFormat::eTGA;
			else if (extension == "hdr")
				return TextureFormat::eHDR;
			else if (extension == "pic")
				return TextureFormat::ePIC;
			else
				return TextureFormat::eNotSupported;
		}

		TextureFormat GetTextureFormat(std::wstring const& path)
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


TEXTURE_HANDLE TextureManager::LoadTexture(std::wstring const& name)
{
	TextureFormat format = GetTextureFormat(name);

	switch (format)
	{
	case TextureFormat::eDDS:
		return LoadDDSTexture(name);
	case TextureFormat::eBMP:
	case TextureFormat::ePNG:
	case TextureFormat::eJPG:
	case TextureFormat::eTIFF:
	case TextureFormat::eGIF:
	case TextureFormat::eICO:
		return LoadWICTexture(name);
	case TextureFormat::eTGA:
	case TextureFormat::eHDR:
	case TextureFormat::ePIC:
		return LoadTexture_HDR_TGA_PIC(ConvertToNarrow(name));
	case TextureFormat::eNotSupported:
	default:
		ADRIA_ASSERT(false && "Unsupported Texture Format!");
	}

	return INVALID_TEXTURE_HANDLE;
}

TEXTURE_HANDLE TextureManager::LoadTexture(std::string const& name)
{
	return LoadTexture(ConvertToWide(name));
}

TEXTURE_HANDLE TextureManager::LoadCubeMap(std::wstring const& name)
{
	TextureFormat format = GetTextureFormat(name);

	ADRIA_ASSERT(format == TextureFormat::eDDS || format == TextureFormat::eHDR && "Cubemap in one file has to be .dds or .hdr format");

	if (auto it = loaded_textures.find(name); it == loaded_textures.end())
	{
		++handle;
		if (format == TextureFormat::eDDS)
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

			ShaderInfo input{};
			input.shadersource = "Resources\\Shaders\\Deferred\\Equirect2cubeCS.hlsl";
			input.stage = ShaderStage::CS;
			input.entrypoint = "cs_main";
			ShaderUtility::CompileShader(input, blob);
			device->CreateComputeShader(blob.GetPointer(), blob.GetLength(), nullptr, &equirect_to_cube);


			D3D11_SAMPLER_DESC sampDesc = {};
			sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			sampDesc.MinLOD = 0;
			sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
			Microsoft::WRL::ComPtr<ID3D11SamplerState> linear_wrap_sampler;
			BREAK_IF_FAILED(device->CreateSamplerState(&sampDesc, &linear_wrap_sampler));
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

TEXTURE_HANDLE TextureManager::LoadCubeMap(std::array<std::string, 6> const& cubemap_textures)
{
	TextureFormat format = GetTextureFormat(cubemap_textures[0]);
	ADRIA_ASSERT(format == TextureFormat::eJPG || format == TextureFormat::ePNG || format == TextureFormat::eTGA ||
		format == TextureFormat::eBMP || format == TextureFormat::eHDR || format == TextureFormat::ePIC);

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
		device->CreateTexture2D(&desc, nullptr, &tex_ptr);
		for (UINT i = 0; i < cubemap_textures.size(); ++i)
			context->UpdateSubresource(tex_ptr.Get(), D3D11CalcSubresource(0, i, desc.MipLevels), nullptr,
				subresource_data_array[i].pSysMem,
				subresource_data_array[i].SysMemPitch, 1);
	}
	else
	{
		device->CreateTexture2D(&desc, subresource_data_array.data(), &tex_ptr);
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = -1;

	ID3D11ShaderResourceView* pSRV = nullptr;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_ptr;
	HRESULT hr = device->CreateShaderResourceView(tex_ptr.Get(), &srvDesc, &view_ptr);
	BREAK_IF_FAILED(hr);


	if (mipmaps) context->GenerateMips(view_ptr.Get());

	texture_map.insert({ handle, view_ptr });

	return handle;
}

ID3D11ShaderResourceView* TextureManager::GetTextureView(TEXTURE_HANDLE tex_handle) const
{
	if (auto it = texture_map.find(tex_handle); it != texture_map.end())
		return it->second.Get();
	else return nullptr;
}

void TextureManager::SetMipMaps(bool mipmaps)
{
	this->mipmaps = mipmaps;
}



TEXTURE_HANDLE TextureManager::LoadDDSTexture(std::wstring const& name)
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

TEXTURE_HANDLE TextureManager::LoadWICTexture(std::wstring const& name)
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

TEXTURE_HANDLE TextureManager::LoadTexture_HDR_TGA_PIC(std::string const& name)
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
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;// | D3D11_BIND_UNORDERED_ACCESS;
		if (mipmaps)
		{
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
		}

		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex_ptr = nullptr;

		device->CreateTexture2D(&desc, nullptr, tex_ptr.GetAddressOf());

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = -1;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view_ptr = nullptr;

		device->CreateShaderResourceView(tex_ptr.Get(), &srvDesc, view_ptr.GetAddressOf());

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