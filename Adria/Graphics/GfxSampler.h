#pragma once
#include "GfxStates.h"

namespace adria
{
	enum class Filter : uint32
	{
		MIN_MAG_MIP_POINT,
		MIN_MAG_POINT_MIP_LINEAR,
		MIN_POINT_MAG_LINEAR_MIP_POINT,
		MIN_POINT_MAG_MIP_LINEAR,
		MIN_LINEAR_MAG_MIP_POINT,
		MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MIN_MAG_LINEAR_MIP_POINT,
		MIN_MAG_MIP_LINEAR,
		ANISOTROPIC,
		COMPARISON_MIN_MAG_MIP_POINT,
		COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
		COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
		COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
		COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
		COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		COMPARISON_MIN_MAG_MIP_LINEAR,
		COMPARISON_ANISOTROPIC,
		MINIMUM_MIN_MAG_MIP_POINT,
		MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
		MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
		MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
		MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
		MINIMUM_MIN_MAG_MIP_LINEAR,
		MINIMUM_ANISOTROPIC,
		MAXIMUM_MIN_MAG_MIP_POINT,
		MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
		MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
		MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
		MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
		MAXIMUM_MIN_MAG_MIP_LINEAR,
		MAXIMUM_ANISOTROPIC,
	};
	inline constexpr D3D11_FILTER ConvertFilter(Filter filter)
	{
		return D3D11_FILTER_MAXIMUM_ANISOTROPIC;
	}

	enum class TextureAddressMode
	{
		Wrap,
		Mirror,
		Clamp,
		Border,
	};
	inline constexpr D3D11_TEXTURE_ADDRESS_MODE ConvertTextureAddressMode(TextureAddressMode mode)
	{
		return D3D11_TEXTURE_ADDRESS_CLAMP;
	}

	struct GfxSamplerDesc
	{
		Filter filter = Filter::MIN_MAG_MIP_POINT;
		TextureAddressMode addressU = TextureAddressMode::Clamp;
		TextureAddressMode addressV = TextureAddressMode::Clamp;
		TextureAddressMode addressW = TextureAddressMode::Clamp;
		float mip_lod_bias = 0.0f;
		uint32 max_anisotropy = 0;
		GfxComparisonFunc comparison_func = GfxComparisonFunc::Never;
		float border_color[4];
		float min_lod = 0.0f;
		float max_lod = FLT_MAX;
	};

	inline D3D11_SAMPLER_DESC ConvertSamplerDesc(GfxSamplerDesc const& desc)
	{
		return D3D11_SAMPLER_DESC{};
	}
}