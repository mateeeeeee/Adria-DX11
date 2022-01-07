#pragma once
#include <d3d11.h>
#include <wrl.h>
#include "../Core/Definitions.h"
#include "../Core/Macros.h"

namespace adria
{
    struct texture3d_desc_t
    {
        U32 width;
        U32 height;
        U32 depth;
        DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        U32 bind_flags = D3D11_BIND_SHADER_RESOURCE;
        bool generate_mipmaps = false;
        U32 mipmap_count = 0;
    };

    class Texture3D
    {
    public:

        Texture3D() = default;

        Texture3D(ID3D11Device* device, texture3d_desc_t const& desc)
        {
            tex3d_desc.Width = desc.width;
            tex3d_desc.Height = desc.height;
            tex3d_desc.MipLevels = desc.generate_mipmaps ? desc.mipmap_count : 1;
            tex3d_desc.Depth = desc.depth;
            tex3d_desc.Format = desc.format;
            tex3d_desc.MipLevels = desc.generate_mipmaps ? desc.mipmap_count : 1;
            tex3d_desc.Usage = desc.usage;
            tex3d_desc.BindFlags = desc.bind_flags;


            if (desc.generate_mipmaps)
            {
                tex3d_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
                tex3d_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
            }

            ADRIA_ASSERT(!(desc.bind_flags & D3D11_BIND_DEPTH_STENCIL) && "Texture3D cannot have DSV!");

            HRESULT HR = device->CreateTexture3D(&tex3d_desc, nullptr, tex3d.GetAddressOf());
            BREAK_IF_FAILED(HR);

            if (desc.bind_flags & D3D11_BIND_SHADER_RESOURCE)
            {
                D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
                srv_desc.Format = desc.format;
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                srv_desc.Texture3D.MostDetailedMip = 0;
                srv_desc.Texture3D.MipLevels = -1;
                
                HRESULT HR = device->CreateShaderResourceView(tex3d.Get(), &srv_desc, tex_srv.GetAddressOf());
                BREAK_IF_FAILED(HR);
            }

            if (desc.bind_flags & D3D11_BIND_UNORDERED_ACCESS)
            {
                D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
                uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
                uav_desc.Format = desc.format;
                uav_desc.Texture3D.FirstWSlice = 0;
                uav_desc.Texture3D.WSize = tex3d_desc.Depth;
                uav_desc.Texture3D.MipSlice = 0;
                HRESULT HR = device->CreateUnorderedAccessView(tex3d.Get(), &uav_desc, tex_uav.GetAddressOf());
                BREAK_IF_FAILED(HR);
            }

            if (desc.bind_flags & D3D11_BIND_RENDER_TARGET)
            {
                HRESULT HR = device->CreateRenderTargetView(tex3d.Get(), nullptr, tex_rtv.GetAddressOf());
                BREAK_IF_FAILED(HR);
            }


        }

        void UpdateTexture3D(ID3D11DeviceContext* context, void* data, U32 pitch)
        {
            context->UpdateSubresource(tex3d.Get(), 0u, nullptr, data, pitch, 0u);
        }

        void GenerateMips(ID3D11DeviceContext* context)
        {
            ADRIA_ASSERT(tex_srv != nullptr && "Generating Mips for NULL SRV!");
            context->GenerateMips(tex_srv.Get());
        }

        auto SRV() const
        {
            return tex_srv.Get();
        }

        auto RTV() const
        {
            return tex_rtv.Get();
        }

        auto UAV() const
        {
            return tex_uav.Get();
        }

       
        U32 Width() const
        {
            return tex3d_desc.Width;
        }

        U32 Height() const
        {
            return tex3d_desc.Height;
        }

        U32 Depth() const
        {
            return tex3d_desc.Depth;
        }

        DXGI_FORMAT Format() const
        {
            return tex3d_desc.Format;
        }

        ID3D11Texture3D* Resource() const
        {
            return tex3d.Get();
        }

    private:
        D3D11_TEXTURE3D_DESC tex3d_desc{};
        Microsoft::WRL::ComPtr<ID3D11Texture3D> tex3d = nullptr;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tex_srv = nullptr;
        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> tex_uav = nullptr;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> tex_rtv = nullptr;
    };


}