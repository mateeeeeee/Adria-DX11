#pragma once
#include <d3d11.h>

namespace adria
{
	using GfxRenderTarget	 = ID3D11RenderTargetView*;
	using GfxDepthTarget	 = ID3D11DepthStencilView*;
	using GfxShaderResourceRO  = ID3D11ShaderResourceView*;
	using GfxShaderResourceRW = ID3D11UnorderedAccessView*;

	using GfxArcRenderTarget		= Ref<ID3D11RenderTargetView>;
	using GfxArcDepthTarget		= Ref<ID3D11DepthStencilView>;
	using GfxArcShaderResourceRO	= Ref<ID3D11ShaderResourceView>;
	using GfxArcShaderResourceRW = Ref<ID3D11UnorderedAccessView>;
}