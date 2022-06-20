#pragma once
#include <vector>
#include "ResourceCommon.h"

namespace adria
{
	enum ETextureType : uint8
	{
		TextureType_1D,
		TextureType_2D,
		TextureType_3D
	};

	struct TextureDesc
	{
		ETextureType type = TextureType_2D;
		uint32 width = 0;
		uint32 height = 0;
		uint32 depth = 0;
		uint32 array_size = 1;
		uint32 mip_levels = 1;
		uint32 sample_count = 1;
		EResourceUsage usage = EResourceUsage::Default;
		EBindFlag bind_flags = EBindFlag::None;
		ETextureMiscFlag misc_flags = ETextureMiscFlag::None;
		ECpuAccess cpu_access = ECpuAccess::None;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

		std::strong_ordering operator<=>(TextureDesc const& other) const = default;
	};

	struct TextureSubresourceDesc
	{
		uint32 first_slice = 0;
		uint32 slice_count = static_cast<uint32>(-1);
		uint32 first_mip = 0;
		uint32 mip_count = static_cast<uint32>(-1);

		std::strong_ordering operator<=>(TextureSubresourceDesc const& other) const = default;
	};

	using TextureInitialData = D3D11_SUBRESOURCE_DATA;

	class Texture
	{
		constexpr static D3D11_TEXTURE1D_DESC ConvertTextureDesc1D(TextureDesc const& desc)
		{
			ADRIA_ASSERT(desc.type == TextureType_1D);

			D3D11_TEXTURE1D_DESC tex_desc{};
			tex_desc.Width = desc.width;
			tex_desc.MipLevels = desc.mip_levels;
			tex_desc.ArraySize = desc.array_size;
			tex_desc.Format = desc.format;
			tex_desc.Usage = ConvertUsage(desc.usage);
			tex_desc.BindFlags = ParseBindFlags(desc.bind_flags);
			tex_desc.CPUAccessFlags = ParseCPUAccessFlags(desc.cpu_access);
			tex_desc.MiscFlags = ParseMiscFlags(desc.misc_flags);
			return tex_desc;
		}
		constexpr static D3D11_TEXTURE2D_DESC ConvertTextureDesc2D(TextureDesc const& desc)
		{
			ADRIA_ASSERT(desc.type == TextureType_2D);

			D3D11_TEXTURE2D_DESC tex_desc{};
			tex_desc.Width = desc.width;
			tex_desc.Height = desc.height;
			tex_desc.MipLevels = desc.mip_levels;
			tex_desc.ArraySize = desc.array_size;
			tex_desc.Format = desc.format;
			tex_desc.SampleDesc.Count = desc.sample_count;
			tex_desc.SampleDesc.Quality = 0;
			tex_desc.Usage = ConvertUsage(desc.usage);
			tex_desc.BindFlags = ParseBindFlags(desc.bind_flags);
			tex_desc.CPUAccessFlags = ParseCPUAccessFlags(desc.cpu_access);
			tex_desc.MiscFlags = ParseMiscFlags(desc.misc_flags);
			return tex_desc;
		}
		constexpr static D3D11_TEXTURE3D_DESC ConvertTextureDesc3D(TextureDesc const& desc)
		{
			ADRIA_ASSERT(desc.type == TextureType_3D);

			D3D11_TEXTURE3D_DESC tex_desc{};
			tex_desc.Width = desc.width;
			tex_desc.Height = desc.height;
			tex_desc.Depth = desc.depth;
			tex_desc.MipLevels = desc.mip_levels;
			tex_desc.Format = desc.format;
			tex_desc.Usage = ConvertUsage(desc.usage);
			tex_desc.BindFlags = ParseBindFlags(desc.bind_flags);
			tex_desc.CPUAccessFlags = ParseCPUAccessFlags(desc.cpu_access);
			tex_desc.MiscFlags = ParseMiscFlags(desc.misc_flags);
			return tex_desc;
		}
	public:
		Texture(GraphicsDevice* gfx, TextureDesc const& desc, TextureInitialData* initial_data = nullptr)
			: gfx(gfx), desc(desc)
		{
			HRESULT hr = S_OK;
			ID3D11Device* device = gfx->Device();
			switch (desc.type)
			{
			case TextureType_1D:
			{
				D3D11_TEXTURE1D_DESC _desc = ConvertTextureDesc1D(desc);
				hr = device->CreateTexture1D(&_desc, initial_data, (ID3D11Texture1D**)resource.ReleaseAndGetAddressOf());
			}
			break;
			case TextureType_2D:
			{
				D3D11_TEXTURE2D_DESC _desc = ConvertTextureDesc2D(desc);
				hr = device->CreateTexture2D(&_desc, initial_data, (ID3D11Texture2D**)resource.ReleaseAndGetAddressOf());
			}
			break;
			case TextureType_3D:
			{
				D3D11_TEXTURE3D_DESC _desc = ConvertTextureDesc3D(desc);
				hr = device->CreateTexture3D(&_desc, initial_data, (ID3D11Texture3D**)resource.ReleaseAndGetAddressOf());
			}
			break;
			default:
				ADRIA_ASSERT(false);
				break;
			}

			if (desc.mip_levels == 0)
			{
				const_cast<TextureDesc&>(desc).mip_levels = (uint32)log2(std::max(desc.width, desc.height)) + 1;
			}
		}

		Texture(Texture const&) = delete;
		Texture& operator=(Texture const&) = delete;
		Texture(Texture&&) = delete;
		Texture& operator=(Texture&&) = delete;
		~Texture() = default;

		[[maybe_unused]] size_t CreateSubresource_SRV(TextureSubresourceDesc const* desc = nullptr)
		{
			TextureSubresourceDesc _desc = desc ? *desc : TextureSubresourceDesc{};
			return CreateSubresource(SubresourceType_SRV, _desc);
		}
		[[maybe_unused]] size_t CreateSubresource_UAV(TextureSubresourceDesc const* desc = nullptr)
		{
			TextureSubresourceDesc _desc = desc ? *desc : TextureSubresourceDesc{};
			return CreateSubresource(SubresourceType_UAV, _desc);
		}
		[[maybe_unused]] size_t CreateSubresource_RTV(TextureSubresourceDesc const* desc = nullptr)
		{
			TextureSubresourceDesc _desc = desc ? *desc : TextureSubresourceDesc{};
			return CreateSubresource(SubresourceType_RTV, _desc);
		}
		[[maybe_unused]] size_t CreateSubresource_DSV(TextureSubresourceDesc const* desc = nullptr)
		{
			TextureSubresourceDesc _desc = desc ? *desc : TextureSubresourceDesc{};
			return CreateSubresource(SubresourceType_DSV, _desc);
		}

		ID3D11ShaderResourceView* GetSubresource_SRV(size_t i = 0) const { return srvs[i].Get(); }
		ID3D11UnorderedAccessView* GetSubresource_UAV(size_t i = 0) const { return uavs[i].Get(); }
		ID3D11RenderTargetView* GetSubresource_RTV(size_t i = 0) const { return rtvs[i].Get(); }
		ID3D11DepthStencilView* GetSubresource_DSV(size_t i = 0) const { return dsvs[i].Get(); }

		ID3D11Resource* GetNative() const { return resource.Get(); }
		ID3D11Resource* Detach() { return resource.Detach(); }
		TextureDesc const& GetDesc() const { return desc; }

	private:
		GraphicsDevice* gfx;
		Microsoft::WRL::ComPtr<ID3D11Resource> resource;
		TextureDesc desc;
		std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> srvs;
		std::vector<Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>> uavs;
		std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> rtvs;
		std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>> dsvs;

	private:
		[[maybe_unused]] size_t CreateSubresource(ESubresourceType type, TextureSubresourceDesc const& view_desc)
		{
			uint32 firstSlice = view_desc.first_slice;
			uint32 sliceCount = view_desc.slice_count;
			uint32 firstMip = view_desc.first_mip;
			uint32 mipCount = view_desc.mip_count;

			ID3D11Device* device = gfx->Device();
			switch (type)
			{
			case SubresourceType_SRV:
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
				switch (desc.format)
				{
				case DXGI_FORMAT_R16_TYPELESS:
					srv_desc.Format = DXGI_FORMAT_R16_UNORM;
					break;
				case DXGI_FORMAT_R32_TYPELESS:
					srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
					break;
				case DXGI_FORMAT_R24G8_TYPELESS:
					srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
					break;
				case DXGI_FORMAT_R32G8X24_TYPELESS:
					srv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
					break;
				default:
					srv_desc.Format = desc.format;
					break;
				}

				if (desc.type == TextureType_1D)
				{
					if (desc.array_size > 1)
					{
						srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
						srv_desc.Texture1DArray.FirstArraySlice = firstSlice;
						srv_desc.Texture1DArray.ArraySize = sliceCount;
						srv_desc.Texture1DArray.MostDetailedMip = firstMip;
						srv_desc.Texture1DArray.MipLevels = mipCount;
					}
					else
					{
						srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
						srv_desc.Texture1D.MostDetailedMip = firstMip;
						srv_desc.Texture1D.MipLevels = mipCount;
					}
				}
				else if (desc.type == TextureType_2D)
				{
					if (desc.array_size > 1)
					{
						if (HasAnyFlag(desc.misc_flags, ETextureMiscFlag::TextureCube))
						{
							if (desc.array_size > 6)
							{
								srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
								srv_desc.TextureCubeArray.First2DArrayFace = firstSlice;
								srv_desc.TextureCubeArray.NumCubes = std::min(desc.array_size, sliceCount) / 6;
								srv_desc.TextureCubeArray.MostDetailedMip = firstMip;
								srv_desc.TextureCubeArray.MipLevels = mipCount;
							}
							else
							{
								srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
								srv_desc.TextureCube.MostDetailedMip = firstMip;
								srv_desc.TextureCube.MipLevels = mipCount;
							}
						}
						else
						{
							if (desc.sample_count > 1)
							{
								srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
								srv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
								srv_desc.Texture2DMSArray.ArraySize = sliceCount;
							}
							else
							{
								srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
								srv_desc.Texture2DArray.FirstArraySlice = firstSlice;
								srv_desc.Texture2DArray.ArraySize = sliceCount;
								srv_desc.Texture2DArray.MostDetailedMip = firstMip;
								srv_desc.Texture2DArray.MipLevels = mipCount;
							}
						}
					}
					else
					{
						if (desc.sample_count > 1)
						{
							srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
						}
						else
						{
							srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
							srv_desc.Texture2D.MostDetailedMip = firstMip;
							srv_desc.Texture2D.MipLevels = mipCount;
						}
					}
				}
				else if (desc.type == TextureType_3D)
				{
					srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
					srv_desc.Texture3D.MostDetailedMip = firstMip;
					srv_desc.Texture3D.MipLevels = mipCount;
				}

				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
				HRESULT hr = device->CreateShaderResourceView(resource.Get(), &srv_desc, &srv);
				if (SUCCEEDED(hr))
				{
					srvs.push_back(srv);
					return srvs.size() - 1;
				}
				else ADRIA_ASSERT(false && "Failed SRV Creation");
			}
			break;
			case SubresourceType_UAV:
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{};

				switch (desc.format)
				{
				case DXGI_FORMAT_R16_TYPELESS:
					uav_desc.Format = DXGI_FORMAT_R16_UNORM;
					break;
				case DXGI_FORMAT_R32_TYPELESS:
					uav_desc.Format = DXGI_FORMAT_R32_FLOAT;
					break;
				case DXGI_FORMAT_R24G8_TYPELESS:
					uav_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
					break;
				case DXGI_FORMAT_R32G8X24_TYPELESS:
					uav_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
					break;
				default:
					uav_desc.Format = desc.format;
					break;
				}

				if (desc.type == TextureType_1D)
				{
					if (desc.array_size > 1)
					{
						uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
						uav_desc.Texture1DArray.FirstArraySlice = firstSlice;
						uav_desc.Texture1DArray.ArraySize = sliceCount;
						uav_desc.Texture1DArray.MipSlice = firstMip;
					}
					else
					{
						uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
						uav_desc.Texture1D.MipSlice = firstMip;
					}
				}
				else if (desc.type == TextureType_2D)
				{
					if (desc.array_size > 1)
					{
						uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
						uav_desc.Texture2DArray.FirstArraySlice = firstSlice;
						uav_desc.Texture2DArray.ArraySize = sliceCount;
						uav_desc.Texture2DArray.MipSlice = firstMip;
					}
					else
					{
						uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
						uav_desc.Texture2D.MipSlice = firstMip;
					}
				}
				else if (desc.type == TextureType_3D)
				{
					uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
					uav_desc.Texture3D.MipSlice = firstMip;
					uav_desc.Texture3D.FirstWSlice = 0;
					uav_desc.Texture3D.WSize = -1;
				}

				Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
				HRESULT hr = device->CreateUnorderedAccessView(resource.Get(), &uav_desc, &uav);
				if (SUCCEEDED(hr))
				{
					uavs.push_back(uav);
					return uavs.size() - 1;
				}
				else ADRIA_ASSERT(false && "Failed UAV Creation");
			}
			break;
			case SubresourceType_RTV:
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtv_desc{};
				switch (desc.format)
				{
				case DXGI_FORMAT_R16_TYPELESS:
					rtv_desc.Format = DXGI_FORMAT_R16_UNORM;
					break;
				case DXGI_FORMAT_R32_TYPELESS:
					rtv_desc.Format = DXGI_FORMAT_R32_FLOAT;
					break;
				case DXGI_FORMAT_R24G8_TYPELESS:
					rtv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
					break;
				case DXGI_FORMAT_R32G8X24_TYPELESS:
					rtv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
					break;
				default:
					rtv_desc.Format = desc.format;
					break;
				}

				if (desc.type == TextureType_1D)
				{
					if (desc.array_size > 1)
					{
						rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
						rtv_desc.Texture1DArray.FirstArraySlice = firstSlice;
						rtv_desc.Texture1DArray.ArraySize = sliceCount;
						rtv_desc.Texture1DArray.MipSlice = firstMip;
					}
					else
					{
						rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
						rtv_desc.Texture1D.MipSlice = firstMip;
					}
				}
				else if (desc.type == TextureType_2D)
				{
					if (desc.array_size > 1)
					{
						if (desc.sample_count > 1)
						{
							rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
							rtv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
							rtv_desc.Texture2DMSArray.ArraySize = sliceCount;
						}
						else
						{
							rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
							rtv_desc.Texture2DArray.FirstArraySlice = firstSlice;
							rtv_desc.Texture2DArray.ArraySize = sliceCount;
							rtv_desc.Texture2DArray.MipSlice = firstMip;
						}
					}
					else
					{
						if (desc.sample_count > 1)
						{
							rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
						}
						else
						{
							rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
							rtv_desc.Texture2D.MipSlice = firstMip;
						}
					}
				}
				else if (desc.type == TextureType_3D)
				{
					rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
					rtv_desc.Texture3D.MipSlice = firstMip;
					rtv_desc.Texture3D.FirstWSlice = 0;
					rtv_desc.Texture3D.WSize = -1;
				}

				Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
				HRESULT hr = device->CreateRenderTargetView(resource.Get(), &rtv_desc, &rtv);
				if (SUCCEEDED(hr))
				{
					rtvs.push_back(rtv);
					return rtvs.size() - 1;
				}
				else ADRIA_ASSERT(false && "Failed RTV Creation");
			}
			break;
			case SubresourceType_DSV:
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
				switch (desc.format)
				{
				case DXGI_FORMAT_R16_TYPELESS:
					dsv_desc.Format = DXGI_FORMAT_D16_UNORM;
					break;
				case DXGI_FORMAT_R32_TYPELESS:
					dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
					break;
				case DXGI_FORMAT_R24G8_TYPELESS:
					dsv_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
					break;
				case DXGI_FORMAT_R32G8X24_TYPELESS:
					dsv_desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
					break;
				default:
					dsv_desc.Format = desc.format;
					break;
				}

				if (desc.type == TextureType_1D)
				{
					if (desc.array_size > 1)
					{
						dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
						dsv_desc.Texture1DArray.FirstArraySlice = firstSlice;
						dsv_desc.Texture1DArray.ArraySize = sliceCount;
						dsv_desc.Texture1DArray.MipSlice = firstMip;
					}
					else
					{
						dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
						dsv_desc.Texture1D.MipSlice = firstMip;
					}
				}
				else if (desc.type == TextureType_2D)
				{
					if (desc.array_size > 1)
					{
						if (desc.sample_count > 1)
						{
							dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
							dsv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
							dsv_desc.Texture2DMSArray.ArraySize = sliceCount;
						}
						else
						{
							dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
							dsv_desc.Texture2DArray.FirstArraySlice = firstSlice;
							dsv_desc.Texture2DArray.ArraySize = sliceCount;
							dsv_desc.Texture2DArray.MipSlice = firstMip;
						}
					}
					else
					{
						if (desc.sample_count > 1)
						{
							dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
						}
						else
						{
							dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
							dsv_desc.Texture2D.MipSlice = firstMip;
						}
					}
				}

				Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv;
				HRESULT hr = device->CreateDepthStencilView(resource.Get(), &dsv_desc, &dsv);
				if (SUCCEEDED(hr))
				{
					dsvs.push_back(dsv);
					return dsvs.size() - 1;
				}
				else ADRIA_ASSERT(false && "Failed DSV Creation");
			}
			break;
			default:
				break;
			}
			return -1;
		}
	};

}