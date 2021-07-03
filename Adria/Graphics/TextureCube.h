#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <array>
#include "../Core/Definitions.h"
#include "../Core/Macros.h"



namespace adria
{

    struct texturecube_desc_t
    {
        u32 width;
        u32 height;
        DXGI_FORMAT format = DXGI_FORMAT_R32_TYPELESS;
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        u32 bind_flags = D3D11_BIND_SHADER_RESOURCE;
        bool generate_mipmaps = false;
        u32 mipmap_count = 0;
        DXGI_FORMAT srv_format = DXGI_FORMAT_R32_FLOAT;
        DXGI_FORMAT dsv_format = DXGI_FORMAT_D32_FLOAT;
        DXGI_FORMAT rtv_format = DXGI_FORMAT_R32_FLOAT;
    };

	class TextureCube
	{

	public:
		TextureCube() = default;

        TextureCube(ID3D11Device* device, texturecube_desc_t const& desc)
        {
            texcube_desc.Width = desc.width;
            texcube_desc.Height = desc.height;
            texcube_desc.MipLevels = desc.generate_mipmaps ? desc.mipmap_count : 1;
            texcube_desc.ArraySize = 6;
            texcube_desc.Format = desc.format;
            texcube_desc.SampleDesc.Count = 1;
            texcube_desc.Usage = desc.usage;
            texcube_desc.BindFlags = desc.bind_flags;
            texcube_desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
            if (desc.generate_mipmaps)
            {
                texcube_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
                texcube_desc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
            }

            HRESULT HR = device->CreateTexture2D(&texcube_desc, nullptr, texcube.GetAddressOf());
            BREAK_IF_FAILED(HR);

            if (texcube_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
            {
                D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                srv_desc.Format = desc.srv_format;
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                srv_desc.Texture2D.MostDetailedMip = 0;
                srv_desc.Texture2D.MipLevels = 1;

                HRESULT HR = device->CreateShaderResourceView(texcube.Get(), &srv_desc, tex_srv.GetAddressOf());
                BREAK_IF_FAILED(HR);
            }

            if (texcube_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
            {
                for (u32 face = 0; face < tex_dsv.size(); ++face)
                {
                    // create target view of depth stensil texture
                    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
                    dsv_desc.Format = desc.dsv_format;
                    dsv_desc.Flags = 0;
                    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsv_desc.Texture2DArray.MipSlice = 0;
                    dsv_desc.Texture2DArray.ArraySize = 1;
                    dsv_desc.Texture2DArray.FirstArraySlice = face;
                    device->CreateDepthStencilView(texcube.Get(), &dsv_desc, tex_dsv[face].GetAddressOf());
                }
            }

            if (texcube_desc.BindFlags & D3D11_BIND_RENDER_TARGET)
            {
                for (u32 face = 0; face < tex_rtv.size(); ++face)
                {
                    // create target view of depth stensil texture
                    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
                    rtv_desc.Format = desc.rtv_format;
                    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtv_desc.Texture2DArray.MipSlice = 0;
                    rtv_desc.Texture2DArray.ArraySize = 1;
                    rtv_desc.Texture2DArray.FirstArraySlice = face;
                    device->CreateRenderTargetView(texcube.Get(), &rtv_desc, tex_rtv[face].GetAddressOf());
                }
            }
        }

        void UpdateTextureCube(ID3D11DeviceContext* context, void* data, u32 pitch)
        {
            context->UpdateSubresource(texcube.Get(), 0u, nullptr, data, pitch, 0u);
        }

        void GenerateMips(ID3D11DeviceContext* context)
        {
            ADRIA_ASSERT(tex_srv != nullptr && "Generating Mips for NULL SRV!");
            context->GenerateMips(tex_srv.Get());
        }

        auto DSV(u32 i) const
        {
            return tex_dsv[i].Get();
        }

        auto RTV(u32 i) const
        {
            return tex_rtv[i].Get();
        }

        auto SRV() const
        {
            return tex_srv.Get();
        }

        ID3D11Texture2D* Resource() const
        {
            return texcube.Get();
        }

	private:
        D3D11_TEXTURE2D_DESC texcube_desc{};
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texcube = nullptr;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tex_srv;
        std::array<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>, 6> tex_dsv;
        std::array<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>, 6> tex_rtv;
        //Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> tex_uav = nullptr;
	};

}

