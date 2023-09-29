#pragma once
#include <vector>
#include "GfxDevice.h"
#include "GfxResourceCommon.h"
#include "GfxView.h"
#include "GfxFormat.h"

namespace adria
{
	enum GfxTextureType : uint8
	{
		TextureType_1D,
		TextureType_2D,
		TextureType_3D
	};

	struct GfxTextureDesc
	{
		GfxTextureType type = TextureType_2D;
		uint32 width = 0;
		uint32 height = 0;
		uint32 depth = 0;
		uint32 array_size = 1;
		uint32 mip_levels = 1;
		uint32 sample_count = 1;
		GfxResourceUsage usage = GfxResourceUsage::Default;
		GfxBindFlag bind_flags = GfxBindFlag::None;
		GfxTextureMiscFlag misc_flags = GfxTextureMiscFlag::None;
		GfxCpuAccess cpu_access = GfxCpuAccess::None;
		GfxFormat format = GfxFormat::UNKNOWN;
		std::strong_ordering operator<=>(GfxTextureDesc const& other) const = default;
	};

	struct GfxTextureSubresourceDesc
	{
		uint32 first_slice = 0;
		uint32 slice_count = static_cast<uint32>(-1);
		uint32 first_mip = 0;
		uint32 mip_count = static_cast<uint32>(-1);

		std::strong_ordering operator<=>(GfxTextureSubresourceDesc const& other) const = default;
	};

	using GfxTextureInitialData = D3D11_SUBRESOURCE_DATA;
	class GfxTexture
	{
		constexpr static D3D11_TEXTURE1D_DESC ConvertTextureDesc1D(GfxTextureDesc const& desc)
		{
			ADRIA_ASSERT(desc.type == TextureType_1D);

			D3D11_TEXTURE1D_DESC tex_desc{};
			tex_desc.Width = desc.width;
			tex_desc.MipLevels = desc.mip_levels;
			tex_desc.ArraySize = desc.array_size;
			tex_desc.Format = ConvertGfxFormat(desc.format);
			tex_desc.Usage = ConvertUsage(desc.usage);
			tex_desc.BindFlags = ParseBindFlags(desc.bind_flags);
			tex_desc.CPUAccessFlags = ParseCPUAccessFlags(desc.cpu_access);
			tex_desc.MiscFlags = ParseMiscFlags(desc.misc_flags);
			return tex_desc;
		}
		constexpr static D3D11_TEXTURE2D_DESC ConvertTextureDesc2D(GfxTextureDesc const& desc)
		{
			ADRIA_ASSERT(desc.type == TextureType_2D);

			D3D11_TEXTURE2D_DESC tex_desc{};
			tex_desc.Width = desc.width;
			tex_desc.Height = desc.height;
			tex_desc.MipLevels = desc.mip_levels;
			tex_desc.ArraySize = desc.array_size;
			tex_desc.Format = ConvertGfxFormat(desc.format);
			tex_desc.SampleDesc.Count = desc.sample_count;
			tex_desc.SampleDesc.Quality = 0;
			tex_desc.Usage = ConvertUsage(desc.usage);
			tex_desc.BindFlags = ParseBindFlags(desc.bind_flags);
			tex_desc.CPUAccessFlags = ParseCPUAccessFlags(desc.cpu_access);
			tex_desc.MiscFlags = ParseMiscFlags(desc.misc_flags);
			return tex_desc;
		}
		constexpr static D3D11_TEXTURE3D_DESC ConvertTextureDesc3D(GfxTextureDesc const& desc)
		{
			ADRIA_ASSERT(desc.type == TextureType_3D);

			D3D11_TEXTURE3D_DESC tex_desc{};
			tex_desc.Width = desc.width;
			tex_desc.Height = desc.height;
			tex_desc.Depth = desc.depth;
			tex_desc.MipLevels = desc.mip_levels;
			tex_desc.Format = ConvertGfxFormat(desc.format);
			tex_desc.Usage = ConvertUsage(desc.usage);
			tex_desc.BindFlags = ParseBindFlags(desc.bind_flags);
			tex_desc.CPUAccessFlags = ParseCPUAccessFlags(desc.cpu_access);
			tex_desc.MiscFlags = ParseMiscFlags(desc.misc_flags);
			return tex_desc;
		}
	public:
		GfxTexture(GfxDevice* gfx, GfxTextureDesc const& desc, GfxTextureInitialData* initial_data = nullptr)
			: gfx(gfx), desc(desc)
		{
			HRESULT hr = S_OK;
			ID3D11Device* device = gfx->GetDevice();

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
				const_cast<GfxTextureDesc&>(desc).mip_levels = (uint32)log2(std::max(desc.width, desc.height)) + 1;
			}

			if (HasAnyFlag(desc.bind_flags, GfxBindFlag::RenderTarget)) CreateRTV();
			if (HasAnyFlag(desc.bind_flags, GfxBindFlag::ShaderResource)) CreateSRV();
			if (HasAnyFlag(desc.bind_flags, GfxBindFlag::DepthStencil)) CreateDSV();
			if (HasAnyFlag(desc.bind_flags, GfxBindFlag::UnorderedAccess)) CreateUAV();
		}
		GfxTexture(GfxTexture const&) = delete;
		GfxTexture& operator=(GfxTexture const&) = delete;
		GfxTexture(GfxTexture&&) = delete;
		GfxTexture& operator=(GfxTexture&&) = delete;
		~GfxTexture() = default;

		[[maybe_unused]] uint64 CreateSRV(GfxTextureSubresourceDesc const* desc = nullptr)
		{
			GfxTextureSubresourceDesc _desc = desc ? *desc : GfxTextureSubresourceDesc{};
			return CreateDescriptor(GfxSubresourceType_SRV, _desc);
		}
		[[maybe_unused]] uint64 CreateUAV(GfxTextureSubresourceDesc const* desc = nullptr)
		{
			GfxTextureSubresourceDesc _desc = desc ? *desc : GfxTextureSubresourceDesc{};
			return CreateDescriptor(GfxSubresourceType_UAV, _desc);
		}
		[[maybe_unused]] uint64 CreateRTV(GfxTextureSubresourceDesc const* desc = nullptr)
		{
			GfxTextureSubresourceDesc _desc = desc ? *desc : GfxTextureSubresourceDesc{};
			return CreateDescriptor(GfxSubresourceType_RTV, _desc);
		}
		[[maybe_unused]] uint64 CreateDSV(GfxTextureSubresourceDesc const* desc = nullptr)
		{
			GfxTextureSubresourceDesc _desc = desc ? *desc : GfxTextureSubresourceDesc{};
			return CreateDescriptor(GfxSubresourceType_DSV, _desc);
		}

		GfxShaderResourceRO	SRV(uint64 i = 0) const { return srvs[i].Get(); }
		GfxShaderResourceRW	UAV(uint64 i = 0) const { return uavs[i].Get(); }
		GfxRenderTarget		RTV(uint64 i = 0) const { return rtvs[i].Get(); }
		GfxDepthTarget		DSV(uint64 i = 0) const { return dsvs[i].Get(); }

		ID3D11Resource* GetNative() const { return resource.Get(); }
		ID3D11Resource* Detach() { return resource.Detach(); }
		GfxTextureDesc const& GetDesc() const { return desc; }

	private:
		GfxDevice* gfx;
		ArcPtr<ID3D11Resource> resource;
		GfxTextureDesc desc;
		std::vector<GfxArcShaderResourceRO> srvs;
		std::vector<GfxArcShaderResourceRW> uavs;
		std::vector<GfxArcRenderTarget> rtvs;
		std::vector<GfxArcDepthTarget> dsvs;

	private:
		[[maybe_unused]] uint64 CreateDescriptor(GfxSubresourceType type, GfxTextureSubresourceDesc const& view_desc)
		{
			uint32 first_slice = view_desc.first_slice;
			uint32 slice_count = view_desc.slice_count;
			uint32 first_mip = view_desc.first_mip;
			uint32 mip_count = view_desc.mip_count;

			ID3D11Device* device = gfx->GetDevice();
			DXGI_FORMAT format = ConvertGfxFormat(desc.format);
			switch (type)
			{
			case GfxSubresourceType_SRV:
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
				switch (format)
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
					srv_desc.Format = format;
					break;
				}

				if (desc.type == TextureType_1D)
				{
					if (desc.array_size > 1)
					{
						srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
						srv_desc.Texture1DArray.FirstArraySlice = first_slice;
						srv_desc.Texture1DArray.ArraySize = slice_count;
						srv_desc.Texture1DArray.MostDetailedMip = first_mip;
						srv_desc.Texture1DArray.MipLevels = mip_count;
					}
					else
					{
						srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
						srv_desc.Texture1D.MostDetailedMip = first_mip;
						srv_desc.Texture1D.MipLevels = mip_count;
					}
				}
				else if (desc.type == TextureType_2D)
				{
					if (desc.array_size > 1)
					{
						if (HasAnyFlag(desc.misc_flags, GfxTextureMiscFlag::TextureCube))
						{
							if (desc.array_size > 6)
							{
								srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
								srv_desc.TextureCubeArray.First2DArrayFace = first_slice;
								srv_desc.TextureCubeArray.NumCubes = std::min(desc.array_size, slice_count) / 6;
								srv_desc.TextureCubeArray.MostDetailedMip = first_mip;
								srv_desc.TextureCubeArray.MipLevels = mip_count;
							}
							else
							{
								srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
								srv_desc.TextureCube.MostDetailedMip = first_mip;
								srv_desc.TextureCube.MipLevels = mip_count;
							}
						}
						else
						{
							if (desc.sample_count > 1)
							{
								srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
								srv_desc.Texture2DMSArray.FirstArraySlice = first_slice;
								srv_desc.Texture2DMSArray.ArraySize = slice_count;
							}
							else
							{
								srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
								srv_desc.Texture2DArray.FirstArraySlice = first_slice;
								srv_desc.Texture2DArray.ArraySize = slice_count;
								srv_desc.Texture2DArray.MostDetailedMip = first_mip;
								srv_desc.Texture2DArray.MipLevels = mip_count;
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
							srv_desc.Texture2D.MostDetailedMip = first_mip;
							srv_desc.Texture2D.MipLevels = mip_count;
						}
					}
				}
				else if (desc.type == TextureType_3D)
				{
					srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
					srv_desc.Texture3D.MostDetailedMip = first_mip;
					srv_desc.Texture3D.MipLevels = mip_count;
				}

				ArcPtr<ID3D11ShaderResourceView> srv;
				HRESULT hr = device->CreateShaderResourceView(resource.Get(), &srv_desc, srv.GetAddressOf());
				if (SUCCEEDED(hr))
				{
					srvs.push_back(srv);
					return srvs.size() - 1;
				}
				else ADRIA_ASSERT(false && "Failed SRV Creation");
			}
			break;
			case GfxSubresourceType_UAV:
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
				switch (format)
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
					uav_desc.Format = format;
					break;
				}

				if (desc.type == TextureType_1D)
				{
					if (desc.array_size > 1)
					{
						uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
						uav_desc.Texture1DArray.FirstArraySlice = first_slice;
						uav_desc.Texture1DArray.ArraySize = slice_count;
						uav_desc.Texture1DArray.MipSlice = first_mip;
					}
					else
					{
						uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
						uav_desc.Texture1D.MipSlice = first_mip;
					}
				}
				else if (desc.type == TextureType_2D)
				{
					if (desc.array_size > 1)
					{
						uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
						uav_desc.Texture2DArray.FirstArraySlice = first_slice;
						uav_desc.Texture2DArray.ArraySize = slice_count;
						uav_desc.Texture2DArray.MipSlice = first_mip;
					}
					else
					{
						uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
						uav_desc.Texture2D.MipSlice = first_mip;
					}
				}
				else if (desc.type == TextureType_3D)
				{
					uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
					uav_desc.Texture3D.MipSlice = first_mip;
					uav_desc.Texture3D.FirstWSlice = 0;
					uav_desc.Texture3D.WSize = -1;
				}

				ArcPtr<ID3D11UnorderedAccessView> uav;
				HRESULT hr = device->CreateUnorderedAccessView(resource.Get(), &uav_desc, uav.GetAddressOf());
				if (SUCCEEDED(hr))
				{
					uavs.push_back(uav);
					return uavs.size() - 1;
				}
				else ADRIA_ASSERT(false && "Failed UAV Creation");
			}
			break;
			case GfxSubresourceType_RTV:
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtv_desc{};
				switch (format)
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
					rtv_desc.Format = format;
					break;
				}

				if (desc.type == TextureType_1D)
				{
					if (desc.array_size > 1)
					{
						rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
						rtv_desc.Texture1DArray.FirstArraySlice = first_slice;
						rtv_desc.Texture1DArray.ArraySize = slice_count;
						rtv_desc.Texture1DArray.MipSlice = first_mip;
					}
					else
					{
						rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
						rtv_desc.Texture1D.MipSlice = first_mip;
					}
				}
				else if (desc.type == TextureType_2D)
				{
					if (desc.array_size > 1)
					{
						if (desc.sample_count > 1)
						{
							rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
							rtv_desc.Texture2DMSArray.FirstArraySlice = first_slice;
							rtv_desc.Texture2DMSArray.ArraySize = slice_count;
						}
						else
						{
							rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
							rtv_desc.Texture2DArray.FirstArraySlice = first_slice;
							rtv_desc.Texture2DArray.ArraySize = slice_count;
							rtv_desc.Texture2DArray.MipSlice = first_mip;
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
							rtv_desc.Texture2D.MipSlice = first_mip;
						}
					}
				}
				else if (desc.type == TextureType_3D)
				{
					rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
					rtv_desc.Texture3D.MipSlice = first_mip;
					rtv_desc.Texture3D.FirstWSlice = 0;
					rtv_desc.Texture3D.WSize = -1;
				}

				ArcPtr<ID3D11RenderTargetView> rtv;
				HRESULT hr = device->CreateRenderTargetView(resource.Get(), &rtv_desc, rtv.GetAddressOf());
				if (SUCCEEDED(hr))
				{
					rtvs.push_back(rtv);
					return rtvs.size() - 1;
				}
				else ADRIA_ASSERT(false && "Failed RTV Creation");
			}
			break;
			case GfxSubresourceType_DSV:
			{
				D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
				switch (format)
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
					dsv_desc.Format = format;
					break;
				}

				if (desc.type == TextureType_1D)
				{
					if (desc.array_size > 1)
					{
						dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
						dsv_desc.Texture1DArray.FirstArraySlice = first_slice;
						dsv_desc.Texture1DArray.ArraySize = slice_count;
						dsv_desc.Texture1DArray.MipSlice = first_mip;
					}
					else
					{
						dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
						dsv_desc.Texture1D.MipSlice = first_mip;
					}
				}
				else if (desc.type == TextureType_2D)
				{
					if (desc.array_size > 1)
					{
						if (desc.sample_count > 1)
						{
							dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
							dsv_desc.Texture2DMSArray.FirstArraySlice = first_slice;
							dsv_desc.Texture2DMSArray.ArraySize = slice_count;
						}
						else
						{
							dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
							dsv_desc.Texture2DArray.FirstArraySlice = first_slice;
							dsv_desc.Texture2DArray.ArraySize = slice_count;
							dsv_desc.Texture2DArray.MipSlice = first_mip;
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
							dsv_desc.Texture2D.MipSlice = first_mip;
						}
					}
				}

				ArcPtr<ID3D11DepthStencilView> dsv;
				HRESULT hr = device->CreateDepthStencilView(resource.Get(), &dsv_desc, dsv.GetAddressOf());
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