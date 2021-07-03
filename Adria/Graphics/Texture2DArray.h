#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <vector>
#include "../Core/Definitions.h"
#include "../Core/Macros.h"

namespace adria
{

    struct texture2darray_desc_t
    {
        u32 width;
        u32 height;
        u32 array_size;
        DXGI_FORMAT format = DXGI_FORMAT_R32_TYPELESS;
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        u32 bind_flags = D3D11_BIND_SHADER_RESOURCE;
        bool generate_mipmaps = false;
        u32 mipmap_count = 0;
        bool single_rtv = false;
        bool single_dsv = false;
        DXGI_FORMAT srv_format = DXGI_FORMAT_R32_FLOAT;
        DXGI_FORMAT dsv_format = DXGI_FORMAT_D32_FLOAT;
        DXGI_FORMAT rtv_format = DXGI_FORMAT_R32_FLOAT;
    };


	class Texture2DArray
	{
	public:
        Texture2DArray() = default;

        Texture2DArray(ID3D11Device* device, texture2darray_desc_t const& desc)
        {
            D3D11_TEXTURE2D_DESC tex2darray_desc{};
            tex2darray_desc.Width = desc.width;
            tex2darray_desc.Height = desc.height;
            tex2darray_desc.MipLevels = 1;
            tex2darray_desc.ArraySize = desc.array_size;
            tex2darray_desc.Format = desc.format;
            tex2darray_desc.SampleDesc.Count = 1;
            tex2darray_desc.SampleDesc.Quality = 0;
            tex2darray_desc.Usage = desc.usage;
            tex2darray_desc.BindFlags = desc.bind_flags;
            tex2darray_desc.CPUAccessFlags = 0;
            tex2darray_desc.MiscFlags = 0;
            
            HRESULT hr = device->CreateTexture2D(&tex2darray_desc, nullptr, tex2darray.GetAddressOf());

            if (desc.generate_mipmaps)
            {
                tex2darray_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
                tex2darray_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
            }

            if (tex2darray_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
            {
                D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
                srv_desc.Format = desc.srv_format;
                srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                srv_desc.Texture2DArray.FirstArraySlice = 0;
                srv_desc.Texture2DArray.ArraySize = tex2darray_desc.ArraySize;
                srv_desc.Texture2DArray.MipLevels = 1;
                srv_desc.Texture2DArray.MostDetailedMip = 0;


                HRESULT HR = device->CreateShaderResourceView(tex2darray.Get(), &srv_desc, tex_srv.GetAddressOf());
                BREAK_IF_FAILED(HR);
            }

            if (tex2darray_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
            {

                if (desc.single_dsv)
                {
                    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                    dsv_desc.Flags = 0;
                    dsv_desc.Format = desc.dsv_format;
                    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsv_desc.Texture2DArray.ArraySize = desc.array_size;
                    dsv_desc.Texture2DArray.MipSlice = 0;
                    dsv_desc.Texture2DArray.FirstArraySlice = 0;

                    tex_dsv.resize(1);
                    hr = device->CreateDepthStencilView(tex2darray.Get(), &dsv_desc, tex_dsv[0].GetAddressOf());

                }
                else
                {
                    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                    dsv_desc.Flags = 0;
                    dsv_desc.Format = desc.dsv_format;
                    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsv_desc.Texture2DArray.ArraySize = 1;
                    dsv_desc.Texture2DArray.MipSlice = 0;

                    tex_dsv.resize(tex2darray_desc.ArraySize);
                    for (u32 i = 0; i < tex2darray_desc.ArraySize; ++i)
                    {
                        dsv_desc.Texture2DArray.FirstArraySlice = D3D11CalcSubresource(0, i, 1);

                        hr = device->CreateDepthStencilView(tex2darray.Get(), &dsv_desc, tex_dsv[i].GetAddressOf());
                    }
                }

                
            }

            if (tex2darray_desc.BindFlags & D3D11_BIND_RENDER_TARGET)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtv_desc{};

                if (desc.single_rtv)
                {
                    rtv_desc.Format = desc.rtv_format;
                    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtv_desc.Texture2DArray.ArraySize = desc.array_size;
                    rtv_desc.Texture2DArray.FirstArraySlice = 0;
                    rtv_desc.Texture2DArray.MipSlice = 0;

                    tex_rtv.resize(1);

                    hr = device->CreateRenderTargetView(tex2darray.Get(), &rtv_desc, tex_rtv[0].GetAddressOf());
                }
                else
                {

                    rtv_desc.Format = desc.rtv_format;
                    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtv_desc.Texture2DArray.ArraySize = 1;
                    rtv_desc.Texture2DArray.MipSlice = 0;

                    tex_rtv.resize(tex2darray_desc.ArraySize);
                    for (u32 i = 0; i < tex2darray_desc.ArraySize; ++i)
                    {
                        rtv_desc.Texture2DArray.FirstArraySlice = D3D11CalcSubresource(0, i, 1);

                        hr = device->CreateRenderTargetView(tex2darray.Get(), &rtv_desc, tex_rtv[i].GetAddressOf());
                    }
                }

            }
        }

        void UpdateTexture2DArray(ID3D11DeviceContext* context, void* data, u32 pitch)
        {
            context->UpdateSubresource(tex2darray.Get(), 0u, nullptr, data, pitch, 0u);
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
            return tex2darray.Get();
        }

	private:
        D3D11_TEXTURE2D_DESC tex2darray_desc{};
        Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2darray = nullptr;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tex_srv = nullptr;
        std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>> tex_dsv;
        std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> tex_rtv;
	};
}