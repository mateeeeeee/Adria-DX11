#pragma once
#include <d3d11.h>

namespace adria
{

    //use namespaces
    class CommonStates
    {
    public:
        static D3D11_BLEND_DESC Opaque()
        {
            return CreateBlendState(D3D11_BLEND_ONE, D3D11_BLEND_ZERO);
        }
        static D3D11_BLEND_DESC AlphaBlend()
        {
            return CreateBlendState(D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA);
        }
        static D3D11_BLEND_DESC Additive()
        {
            return CreateBlendState(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE); //D3D11_BLEND_ONE, D3D11_BLEND_ONE
        }

        static D3D11_DEPTH_STENCIL_DESC DepthNone()
        {
            return CreateDepthState(false, false);
        }
        static D3D11_DEPTH_STENCIL_DESC DepthDefault()
        {
            return CreateDepthState(true, true);
        }
        static D3D11_DEPTH_STENCIL_DESC DepthRead()
        {
            return CreateDepthState(false, false);
        }

        static D3D11_RASTERIZER_DESC CullNone()
        {
            return CreateRasterizerState(D3D11_CULL_NONE, D3D11_FILL_SOLID);
        }
        static D3D11_RASTERIZER_DESC CullClockwise()
        {
            return CreateRasterizerState(D3D11_CULL_FRONT, D3D11_FILL_SOLID);
        }
        static D3D11_RASTERIZER_DESC CullCounterClockwise()
        {
            return CreateRasterizerState(D3D11_CULL_BACK, D3D11_FILL_SOLID);
        }
        static D3D11_RASTERIZER_DESC Wireframe()
        {
            return CreateRasterizerState(D3D11_CULL_NONE, D3D11_FILL_WIREFRAME);
        }

        static D3D11_SAMPLER_DESC PointWrap()
        {
            return CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP);
        }
        static D3D11_SAMPLER_DESC PointClamp()
        {
            return CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
        }
        static D3D11_SAMPLER_DESC LinearWrap()
        {
            return CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);
        }
        static D3D11_SAMPLER_DESC LinearClamp()
        {
            return CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
        }
        static D3D11_SAMPLER_DESC AnisotropicWrap()
        {
            return CreateSamplerState(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP);
        }
        static D3D11_SAMPLER_DESC AnisotropicClamp()
        {
            return CreateSamplerState(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_CLAMP);
        }

        static D3D11_SAMPLER_DESC ComparisonSampler()
        {
            D3D11_SAMPLER_DESC comparisonSamplerDesc{};
            comparisonSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
            comparisonSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
            comparisonSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
            comparisonSamplerDesc.BorderColor[0] = 1.0f;
            comparisonSamplerDesc.BorderColor[1] = 1.0f;
            comparisonSamplerDesc.BorderColor[2] = 1.0f;
            comparisonSamplerDesc.BorderColor[3] = 1.0f;
            comparisonSamplerDesc.MinLOD = 0.f;
            comparisonSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
            comparisonSamplerDesc.MipLODBias = 0.f;
            comparisonSamplerDesc.MaxAnisotropy = 0;
            comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
            comparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;

            return comparisonSamplerDesc;
        }

    private:

        static D3D11_BLEND_DESC CreateBlendState(D3D11_BLEND srcBlend, D3D11_BLEND destBlend)
        {
            D3D11_BLEND_DESC desc = {};

            desc.RenderTarget[0].BlendEnable = (srcBlend != D3D11_BLEND_ONE) ||
                (destBlend != D3D11_BLEND_ZERO);

            desc.RenderTarget[0].SrcBlend = desc.RenderTarget[0].SrcBlendAlpha = srcBlend;
            desc.RenderTarget[0].DestBlend = desc.RenderTarget[0].DestBlendAlpha = destBlend;
            desc.RenderTarget[0].BlendOp = desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

            return desc;
        }

        static D3D11_DEPTH_STENCIL_DESC CreateDepthState(bool enable, bool writeEnable)
        {
            D3D11_DEPTH_STENCIL_DESC desc = {};

            desc.DepthEnable = enable ? TRUE : FALSE;
            desc.DepthWriteMask = writeEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
            desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

            desc.StencilEnable = FALSE;
            desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
            desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

            desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
            desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
            desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

            desc.BackFace = desc.FrontFace;

            return desc;
        }

        static D3D11_RASTERIZER_DESC CreateRasterizerState(D3D11_CULL_MODE cullMode, D3D11_FILL_MODE fillMode)
        {
            D3D11_RASTERIZER_DESC desc = {};

            desc.CullMode = cullMode;
            desc.FillMode = fillMode;
            desc.DepthClipEnable = TRUE;
            desc.MultisampleEnable = TRUE;

            return desc;
        }

        static D3D11_SAMPLER_DESC CreateSamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode)
        {
            D3D11_SAMPLER_DESC desc = {};

            desc.Filter = filter;

            desc.AddressU = addressMode;
            desc.AddressV = addressMode;
            desc.AddressW = addressMode;

            desc.MaxAnisotropy = D3D11_MAX_MAXANISOTROPY;

            desc.MaxLOD = D3D11_FLOAT32_MAX;
            desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

            return desc;
        };
    };
}