#pragma once
#include <d3d11.h>
#include <wrl.h>
#include "../Core/Definitions.h"
#include "../Core/Macros.h"

namespace adria
{

    struct texture2d_srv_desc_t
    {
        u32 most_detailed_mip;
        u32 mip_levels;
        DXGI_FORMAT format;
    };

    struct texture2d_uav_desc_t
    {
        u32 mip_slice;
    };

    struct texture2d_rtv_desc_t
    {
        u32 mip_slice;
    };

    struct texture2d_dsv_desc_t
    {
        u32 mip_slice;
        DXGI_FORMAT depth_format;
        bool readonly;
    };

    struct init_data_t
    {
        void* data = nullptr;
        u32 pitch = 0;
    };

    struct texture2d_desc_t
    {
        u32 width;
        u32 height;
        DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        u32 bind_flags = D3D11_BIND_SHADER_RESOURCE;
        bool generate_mipmaps = false;
        u32 mipmap_count = 0;
        init_data_t init_data{};
        texture2d_srv_desc_t srv_desc = { 0, u32(-1), DXGI_FORMAT_R16G16B16A16_FLOAT };
        texture2d_uav_desc_t uav_desc = { 0 };
        texture2d_rtv_desc_t rtv_desc = { 0 };
        texture2d_dsv_desc_t dsv_desc = { 0, DXGI_FORMAT_D32_FLOAT, false};
    };

    class Texture2D
    {
    public:

        Texture2D() = default;

        Texture2D(ID3D11Device* device, texture2d_desc_t const& desc)
        {
            tex2d_desc.Width = desc.width;
            tex2d_desc.Height = desc.height;
            tex2d_desc.MipLevels = desc.generate_mipmaps ? desc.mipmap_count : 1;
            tex2d_desc.ArraySize = 1;
            tex2d_desc.Format = desc.format;
            tex2d_desc.SampleDesc.Count = 1;
            tex2d_desc.Usage = desc.usage;
            tex2d_desc.BindFlags = desc.bind_flags; 
            if (desc.generate_mipmaps)
            {
                tex2d_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
                tex2d_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
            }

            if (desc.init_data.data != nullptr)
            {
                D3D11_SUBRESOURCE_DATA data{};
                data.pSysMem = desc.init_data.data;
                data.SysMemPitch = desc.init_data.pitch;
                data.SysMemSlicePitch = 0;

                HRESULT HR = device->CreateTexture2D(&tex2d_desc, &data, tex2d.GetAddressOf());
                BREAK_IF_FAILED(HR);
            }
            else
            {
                HRESULT HR = device->CreateTexture2D(&tex2d_desc, nullptr, tex2d.GetAddressOf());
                BREAK_IF_FAILED(HR);
            }



            if (desc.bind_flags & D3D11_BIND_DEPTH_STENCIL)
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                dsv_desc.Format = desc.dsv_desc.depth_format;
                dsv_desc.Texture2D.MipSlice = desc.dsv_desc.mip_slice;
                dsv_desc.Flags = desc.dsv_desc.readonly ? D3D11_DSV_READ_ONLY_DEPTH : 0;

                HRESULT HR = device->CreateDepthStencilView(tex2d.Get(), &dsv_desc, tex_dsv.GetAddressOf());
                BREAK_IF_FAILED(HR);
            }

            if (desc.bind_flags & D3D11_BIND_SHADER_RESOURCE)
            {
                D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
                srv_desc.Format = desc.srv_desc.format;
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srv_desc.Texture2D.MostDetailedMip = desc.srv_desc.most_detailed_mip;
                srv_desc.Texture2D.MipLevels = desc.srv_desc.mip_levels;

                HRESULT HR = device->CreateShaderResourceView(tex2d.Get(), &srv_desc, tex_srv.GetAddressOf());
                BREAK_IF_FAILED(HR);
            }

            if (desc.bind_flags & D3D11_BIND_UNORDERED_ACCESS)
            {
                D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
                uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                uav_desc.Format = tex2d_desc.Format;
                uav_desc.Texture2D.MipSlice = desc.uav_desc.mip_slice;
                HRESULT HR = device->CreateUnorderedAccessView(tex2d.Get(), &uav_desc, tex_uav.GetAddressOf());
                BREAK_IF_FAILED(HR);
            }

            if (desc.bind_flags & D3D11_BIND_RENDER_TARGET)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtv_desc{};
                rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                rtv_desc.Format = tex2d_desc.Format;
                rtv_desc.Texture2D.MipSlice = desc.rtv_desc.mip_slice;
                HRESULT HR = device->CreateRenderTargetView(tex2d.Get(), &rtv_desc, tex_rtv.GetAddressOf());
                BREAK_IF_FAILED(HR);
            }

            
        }

        void UpdateTexture2D(ID3D11DeviceContext* context, void* data, u32 pitch)
        {
            context->UpdateSubresource(tex2d.Get(), 0u, nullptr, data, pitch, 0u);
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

        auto DSV() const
        {
            return tex_dsv.Get();
        }

        u32 Width() const
        {
            return tex2d_desc.Width;
        }

        u32 Height() const
        {
            return tex2d_desc.Height;
        }

        DXGI_FORMAT Format() const
        {
            return tex2d_desc.Format;
        }

        ID3D11Texture2D* Resource() const
        {
            return tex2d.Get();
        }

    private:
        D3D11_TEXTURE2D_DESC tex2d_desc{};
        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2d             = nullptr;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tex_srv  = nullptr;
        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> tex_uav = nullptr;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> tex_dsv    = nullptr;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> tex_rtv    = nullptr;
    };
}