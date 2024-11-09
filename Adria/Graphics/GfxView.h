#pragma once
#include <d3d11.h>

namespace adria
{
	using GfxRenderTarget		= ID3D11RenderTargetView*;
	using GfxDepthTarget		= ID3D11DepthStencilView*;
	using GfxShaderResourceRO	= ID3D11ShaderResourceView*;
	using GfxShaderResourceRW	= ID3D11UnorderedAccessView*;

	using GfxRenderTargetRef		= Ref<ID3D11RenderTargetView>;
	using GfxDepthTargetRef			= Ref<ID3D11DepthStencilView>;
	using GfxShaderResourceRORef	= Ref<ID3D11ShaderResourceView>;
	using GfxShaderResourceRWRef	= Ref<ID3D11UnorderedAccessView>;
}