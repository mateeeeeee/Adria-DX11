#pragma once
#include <d3d11.h>

namespace adria
{

    //use namespaces
    class GfxCommonStates
    {
    public:
        static D3D11_BLEND_DESC Opaque();
        static D3D11_BLEND_DESC AlphaBlend();
        static D3D11_BLEND_DESC Additive();

        static D3D11_DEPTH_STENCIL_DESC DepthNone();
        static D3D11_DEPTH_STENCIL_DESC DepthDefault();
        static D3D11_DEPTH_STENCIL_DESC DepthRead();

        static D3D11_RASTERIZER_DESC CullNone();
        static D3D11_RASTERIZER_DESC CullClockwise();
        static D3D11_RASTERIZER_DESC CullCounterClockwise();
        static D3D11_RASTERIZER_DESC Wireframe();

        static D3D11_SAMPLER_DESC PointWrap();
        static D3D11_SAMPLER_DESC PointClamp();
        static D3D11_SAMPLER_DESC LinearWrap();
        static D3D11_SAMPLER_DESC LinearClamp();
        static D3D11_SAMPLER_DESC AnisotropicWrap();
        static D3D11_SAMPLER_DESC AnisotropicClamp();

        static D3D11_SAMPLER_DESC ComparisonSampler();

    private:

        static D3D11_BLEND_DESC CreateBlendState(D3D11_BLEND srcBlend, D3D11_BLEND destBlend);

        static D3D11_DEPTH_STENCIL_DESC CreateDepthState(bool enable, bool writeEnable);

        static D3D11_RASTERIZER_DESC CreateRasterizerState(D3D11_CULL_MODE cullMode, D3D11_FILL_MODE fillMode);

        static D3D11_SAMPLER_DESC CreateSamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode);;
    };
}