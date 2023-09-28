#pragma once
#include <d3d11.h>

namespace adria
{
	using GfxRenderTarget	 = ID3D11RenderTargetView*;
	using GfxDepthTarget	 = ID3D11DepthStencilView*;
	using GfxShaderResourceRO  = ID3D11ShaderResourceView*;
	using GfxShaderResourceRW = ID3D11UnorderedAccessView*;

	using GfxArcRenderTarget		= ArcPtr<ID3D11RenderTargetView>;
	using GfxArcDepthTarget		= ArcPtr<ID3D11DepthStencilView>;
	using GfxArcShaderResourceRO	= ArcPtr<ID3D11ShaderResourceView>;
	using GfxArcShaderResourceRW = ArcPtr<ID3D11UnorderedAccessView>;
}