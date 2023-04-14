#include "GfxCommonStates.h"


namespace adria
{

	D3D11_BLEND_DESC GfxCommonStates::Opaque()
	{
		return CreateBlendState(D3D11_BLEND_ONE, D3D11_BLEND_ZERO);
	}

	D3D11_BLEND_DESC GfxCommonStates::AlphaBlend()
	{
		return CreateBlendState(D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA);
	}

	D3D11_BLEND_DESC GfxCommonStates::Additive()
	{
		return CreateBlendState(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE); //D3D11_BLEND_ONE, D3D11_BLEND_ONE
	}

	D3D11_DEPTH_STENCIL_DESC GfxCommonStates::DepthNone()
	{
		return CreateDepthState(false, false);
	}

	D3D11_DEPTH_STENCIL_DESC GfxCommonStates::DepthDefault()
	{
		return CreateDepthState(true, true);
	}

	D3D11_DEPTH_STENCIL_DESC GfxCommonStates::DepthRead()
	{
		return CreateDepthState(false, false);
	}

	D3D11_RASTERIZER_DESC GfxCommonStates::CullNone()
	{
		return CreateRasterizerState(D3D11_CULL_NONE, D3D11_FILL_SOLID);
	}

	D3D11_RASTERIZER_DESC GfxCommonStates::CullClockwise()
	{
		return CreateRasterizerState(D3D11_CULL_FRONT, D3D11_FILL_SOLID);
	}

	D3D11_RASTERIZER_DESC GfxCommonStates::CullCounterClockwise()
	{
		return CreateRasterizerState(D3D11_CULL_BACK, D3D11_FILL_SOLID);
	}

	D3D11_RASTERIZER_DESC GfxCommonStates::Wireframe()
	{
		return CreateRasterizerState(D3D11_CULL_NONE, D3D11_FILL_WIREFRAME);
	}

	D3D11_SAMPLER_DESC GfxCommonStates::PointWrap()
	{
		return CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP);
	}

	D3D11_SAMPLER_DESC GfxCommonStates::PointClamp()
	{
		return CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
	}

	D3D11_SAMPLER_DESC GfxCommonStates::LinearWrap()
	{
		return CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);
	}

	D3D11_SAMPLER_DESC GfxCommonStates::LinearClamp()
	{
		return CreateSamplerState(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
	}

	D3D11_SAMPLER_DESC GfxCommonStates::AnisotropicWrap()
	{
		return CreateSamplerState(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_WRAP);
	}

	D3D11_SAMPLER_DESC GfxCommonStates::AnisotropicClamp()
	{
		return CreateSamplerState(D3D11_FILTER_ANISOTROPIC, D3D11_TEXTURE_ADDRESS_CLAMP);
	}

	D3D11_SAMPLER_DESC GfxCommonStates::ComparisonSampler()
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

	D3D11_BLEND_DESC GfxCommonStates::CreateBlendState(D3D11_BLEND srcBlend, D3D11_BLEND destBlend)
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

	D3D11_DEPTH_STENCIL_DESC GfxCommonStates::CreateDepthState(bool enable, bool writeEnable)
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

	D3D11_RASTERIZER_DESC GfxCommonStates::CreateRasterizerState(D3D11_CULL_MODE cullMode, D3D11_FILL_MODE fillMode)
	{
		D3D11_RASTERIZER_DESC desc = {};

		desc.CullMode = cullMode;
		desc.FillMode = fillMode;
		desc.DepthClipEnable = TRUE;
		desc.MultisampleEnable = TRUE;

		return desc;
	}

	D3D11_SAMPLER_DESC GfxCommonStates::CreateSamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addressMode)
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
	}

}