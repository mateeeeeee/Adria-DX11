#include "GfxStates.h"
#include "GfxDevice.h"
#include "Utilities/EnumUtil.h"

namespace adria
{
	namespace
	{
		constexpr D3D11_FILL_MODE ConvertFillMode(GfxFillMode value)
		{
			switch (value)
			{
			case GfxFillMode::Wireframe:
				return D3D11_FILL_WIREFRAME;
				break;
			case GfxFillMode::Solid:
				return D3D11_FILL_SOLID;
				break;
			default:
				break;
			}
			return D3D11_FILL_WIREFRAME;
		}
		constexpr D3D11_CULL_MODE ConvertCullMode(GfxCullMode value)
		{
			switch (value)
			{
			case GfxCullMode::None:
				return D3D11_CULL_NONE;
				break;
			case GfxCullMode::Front:
				return D3D11_CULL_FRONT;
				break;
			case GfxCullMode::Back:
				return D3D11_CULL_BACK;
				break;
			default:
				break;
			}
			return D3D11_CULL_NONE;
		}
		constexpr D3D11_DEPTH_WRITE_MASK ConvertDepthWriteMask(GfxDepthWriteMask value)
		{
			switch (value)
			{
			case GfxDepthWriteMask::Zero:
				return D3D11_DEPTH_WRITE_MASK_ZERO;
				break;
			case GfxDepthWriteMask::All:
				return D3D11_DEPTH_WRITE_MASK_ALL;
				break;
			default:
				break;
			}
			return D3D11_DEPTH_WRITE_MASK_ZERO;
		}
		constexpr D3D11_STENCIL_OP ConvertStencilOp(GfxStencilOp value)
		{
			switch (value)
			{
			case GfxStencilOp::Keep:
				return D3D11_STENCIL_OP_KEEP;
				break;
			case GfxStencilOp::Zero:
				return D3D11_STENCIL_OP_ZERO;
				break;
			case GfxStencilOp::Replace:
				return D3D11_STENCIL_OP_REPLACE;
				break;
			case GfxStencilOp::IncrSat:
				return D3D11_STENCIL_OP_INCR_SAT;
				break;
			case GfxStencilOp::DecrSat:
				return D3D11_STENCIL_OP_DECR_SAT;
				break;
			case GfxStencilOp::Invert:
				return D3D11_STENCIL_OP_INVERT;
				break;
			case GfxStencilOp::Incr:
				return D3D11_STENCIL_OP_INCR;
				break;
			case GfxStencilOp::Decr:
				return D3D11_STENCIL_OP_DECR;
				break;
			default:
				break;
			}
			return D3D11_STENCIL_OP_KEEP;
		}
		constexpr D3D11_BLEND ConvertBlend(GfxBlend value)
		{
			switch (value)
			{
			case GfxBlend::Zero:
				return D3D11_BLEND_ZERO;
			case GfxBlend::One:
				return D3D11_BLEND_ONE;
			case GfxBlend::SrcColor:
				return D3D11_BLEND_SRC_COLOR;
			case GfxBlend::InvSrcColor:
				return D3D11_BLEND_INV_SRC_COLOR;
			case GfxBlend::SrcAlpha:
				return D3D11_BLEND_SRC_ALPHA;
			case GfxBlend::InvSrcAlpha:
				return D3D11_BLEND_INV_SRC_ALPHA;
			case GfxBlend::DstAlpha:
				return D3D11_BLEND_DEST_ALPHA;
			case GfxBlend::InvDstAlpha:
				return D3D11_BLEND_INV_DEST_ALPHA;
			case GfxBlend::DstColor:
				return D3D11_BLEND_DEST_COLOR;
			case GfxBlend::InvDstColor:
				return D3D11_BLEND_INV_DEST_COLOR;
			case GfxBlend::SrcAlphaSat:
				return D3D11_BLEND_SRC_ALPHA_SAT;
			case GfxBlend::BlendFactor:
				return D3D11_BLEND_BLEND_FACTOR;
			case GfxBlend::InvBlendFactor:
				return D3D11_BLEND_INV_BLEND_FACTOR;
			case GfxBlend::Src1Color:
				return D3D11_BLEND_SRC1_COLOR;
			case GfxBlend::InvSrc1Color:
				return D3D11_BLEND_INV_SRC1_COLOR;
			case GfxBlend::Src1Alpha:
				return D3D11_BLEND_SRC1_ALPHA;
			case GfxBlend::InvSrc1Alpha:
				return D3D11_BLEND_INV_SRC1_ALPHA;
			default:
				return D3D11_BLEND_ZERO;
			}
		}
		constexpr D3D11_BLEND ConvertAlphaBlend(GfxBlend value)
		{
			switch (value)
			{
			case GfxBlend::SrcColor:
				return D3D11_BLEND_SRC_ALPHA;
			case GfxBlend::InvSrcColor:
				return D3D11_BLEND_INV_SRC_ALPHA;
			case GfxBlend::DstColor:
				return D3D11_BLEND_DEST_ALPHA;
			case GfxBlend::InvDstColor:
				return D3D11_BLEND_INV_DEST_ALPHA;
			case GfxBlend::Src1Color:
				return D3D11_BLEND_SRC1_ALPHA;
			case GfxBlend::InvSrc1Color:
				return D3D11_BLEND_INV_SRC1_ALPHA;
			default:
				return ConvertBlend(value);
			}
		}
		constexpr D3D11_BLEND_OP ConvertBlendOp(GfxBlendOp value)
		{
			switch (value)
			{
			case GfxBlendOp::Add:
				return D3D11_BLEND_OP_ADD;
			case GfxBlendOp::Subtract:
				return D3D11_BLEND_OP_SUBTRACT;
			case GfxBlendOp::RevSubtract:
				return D3D11_BLEND_OP_REV_SUBTRACT;
			case GfxBlendOp::Min:
				return D3D11_BLEND_OP_MIN;
			case GfxBlendOp::Max:
				return D3D11_BLEND_OP_MAX;
			default:
				return D3D11_BLEND_OP_ADD;
			}
		}
		constexpr uint32 ParseColorWriteMask(GfxColorWrite value)
		{
			uint32 _flag = 0;
			if (value == GfxColorWrite::EnableAll)
			{
				return D3D11_COLOR_WRITE_ENABLE_ALL;
			}
			else
			{
				if (HasAnyFlag(value, GfxColorWrite::EnableRed))
					_flag |= D3D11_COLOR_WRITE_ENABLE_RED;
				if (HasAnyFlag(value, GfxColorWrite::EnableGreen))
					_flag |= D3D11_COLOR_WRITE_ENABLE_GREEN;
				if (HasAnyFlag(value, GfxColorWrite::EnableBlue))
					_flag |= D3D11_COLOR_WRITE_ENABLE_BLUE;
				if (HasAnyFlag(value, GfxColorWrite::EnableAlpha))
					_flag |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
			}
			return _flag;
		}
		constexpr D3D11_COMPARISON_FUNC ConvertComparisonFunc(GfxComparisonFunc value)
		{
			switch (value)
			{
			case GfxComparisonFunc::Never:
				return D3D11_COMPARISON_NEVER;
				break;
			case GfxComparisonFunc::Less:
				return D3D11_COMPARISON_LESS;
				break;
			case GfxComparisonFunc::Equal:
				return D3D11_COMPARISON_EQUAL;
				break;
			case GfxComparisonFunc::LessEqual:
				return D3D11_COMPARISON_LESS_EQUAL;
				break;
			case GfxComparisonFunc::Greater:
				return D3D11_COMPARISON_GREATER;
				break;
			case GfxComparisonFunc::NotEqual:
				return D3D11_COMPARISON_NOT_EQUAL;
				break;
			case GfxComparisonFunc::GreaterEqual:
				return D3D11_COMPARISON_GREATER_EQUAL;
				break;
			case GfxComparisonFunc::Always:
				return D3D11_COMPARISON_ALWAYS;
				break;
			default:
				break;
			}
			return D3D11_COMPARISON_NEVER;
		}
		constexpr D3D11_FILTER ConvertFilter(GfxFilter filter)
		{
			switch (filter)
			{
			case GfxFilter::MIN_MAG_MIP_POINT: return D3D11_FILTER_MIN_MAG_MIP_POINT;	
			case GfxFilter::MIN_MAG_POINT_MIP_LINEAR: return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			case GfxFilter::MIN_POINT_MAG_LINEAR_MIP_POINT: return D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			case GfxFilter::MIN_POINT_MAG_MIP_LINEAR: return D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			case GfxFilter::MIN_LINEAR_MAG_MIP_POINT: return D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			case GfxFilter::MIN_LINEAR_MAG_POINT_MIP_LINEAR: return D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			case GfxFilter::MIN_MAG_LINEAR_MIP_POINT: return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			case GfxFilter::MIN_MAG_MIP_LINEAR: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			case GfxFilter::ANISOTROPIC: return D3D11_FILTER_ANISOTROPIC;
			case GfxFilter::COMPARISON_MIN_MAG_MIP_POINT: return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
			case GfxFilter::COMPARISON_MIN_MAG_POINT_MIP_LINEAR: return D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
			case GfxFilter::COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT: return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
			case GfxFilter::COMPARISON_MIN_POINT_MAG_MIP_LINEAR: return D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
			case GfxFilter::COMPARISON_MIN_LINEAR_MAG_MIP_POINT: return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
			case GfxFilter::COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR: return D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			case GfxFilter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT: return D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			case GfxFilter::COMPARISON_MIN_MAG_MIP_LINEAR: return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
			case GfxFilter::COMPARISON_ANISOTROPIC: return D3D11_FILTER_COMPARISON_ANISOTROPIC;
			case GfxFilter::MINIMUM_MIN_MAG_MIP_POINT: return D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
			case GfxFilter::MINIMUM_MIN_MAG_POINT_MIP_LINEAR: return D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
			case GfxFilter::MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT: return D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
			case GfxFilter::MINIMUM_MIN_POINT_MAG_MIP_LINEAR: return D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
			case GfxFilter::MINIMUM_MIN_LINEAR_MAG_MIP_POINT: return D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
			case GfxFilter::MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR: return D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			case GfxFilter::MINIMUM_MIN_MAG_LINEAR_MIP_POINT: return D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
			case GfxFilter::MINIMUM_MIN_MAG_MIP_LINEAR: return D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
			case GfxFilter::MINIMUM_ANISOTROPIC: return D3D11_FILTER_MINIMUM_ANISOTROPIC;
			case GfxFilter::MAXIMUM_MIN_MAG_MIP_POINT: return D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
			case GfxFilter::MAXIMUM_MIN_MAG_POINT_MIP_LINEAR: return D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
			case GfxFilter::MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT: return D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
			case GfxFilter::MAXIMUM_MIN_POINT_MAG_MIP_LINEAR: return D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
			case GfxFilter::MAXIMUM_MIN_LINEAR_MAG_MIP_POINT: return D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
			case GfxFilter::MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR: return D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			case GfxFilter::MAXIMUM_MIN_MAG_LINEAR_MIP_POINT: return D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
			case GfxFilter::MAXIMUM_MIN_MAG_MIP_LINEAR: return D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
			case GfxFilter::MAXIMUM_ANISOTROPIC: return D3D11_FILTER_MAXIMUM_ANISOTROPIC;
			}
			return D3D11_FILTER_MAXIMUM_ANISOTROPIC;
		}
		constexpr D3D11_TEXTURE_ADDRESS_MODE ConvertTextureAddressMode(GfxTextureAddressMode mode)
		{
			switch (mode)
			{
			case GfxTextureAddressMode::Wrap:
				return D3D11_TEXTURE_ADDRESS_WRAP;
			case GfxTextureAddressMode::Mirror:
				return D3D11_TEXTURE_ADDRESS_MIRROR;
			case GfxTextureAddressMode::Clamp:
				return D3D11_TEXTURE_ADDRESS_CLAMP;
			case GfxTextureAddressMode::Border:
				return D3D11_TEXTURE_ADDRESS_BORDER;
			}
			return D3D11_TEXTURE_ADDRESS_CLAMP;
		}

		D3D11_RASTERIZER_DESC ConvertRasterizerDesc(GfxRasterizerStateDesc _rs)
		{
			D3D11_RASTERIZER_DESC rs{};
			rs.FillMode = ConvertFillMode(_rs.fill_mode);
			rs.CullMode = ConvertCullMode(_rs.cull_mode);
			rs.FrontCounterClockwise = _rs.front_counter_clockwise;
			rs.DepthBias = _rs.depth_bias;
			rs.DepthBiasClamp = _rs.depth_bias_clamp;
			rs.SlopeScaledDepthBias = _rs.slope_scaled_depth_bias;
			rs.DepthClipEnable = _rs.depth_clip_enable;
			rs.MultisampleEnable = _rs.multisample_enable;
			rs.AntialiasedLineEnable = _rs.antialiased_line_enable;
			return rs;
		}
		D3D11_DEPTH_STENCIL_DESC ConvertDepthStencilDesc(GfxDepthStencilStateDesc _dss)
		{
			D3D11_DEPTH_STENCIL_DESC dss{};
			dss.DepthEnable = _dss.depth_enable;
			dss.DepthWriteMask = ConvertDepthWriteMask(_dss.depth_write_mask);
			dss.DepthFunc = ConvertComparisonFunc(_dss.depth_func);
			dss.StencilEnable = _dss.stencil_enable;
			dss.StencilReadMask = _dss.stencil_read_mask;
			dss.StencilWriteMask = _dss.stencil_write_mask;
			dss.FrontFace.StencilDepthFailOp = ConvertStencilOp(_dss.front_face.stencil_depth_fail_op);
			dss.FrontFace.StencilFailOp = ConvertStencilOp(_dss.front_face.stencil_fail_op);
			dss.FrontFace.StencilFunc = ConvertComparisonFunc(_dss.front_face.stencil_func);
			dss.FrontFace.StencilPassOp = ConvertStencilOp(_dss.front_face.stencil_pass_op);
			dss.BackFace.StencilDepthFailOp = ConvertStencilOp(_dss.back_face.stencil_depth_fail_op);
			dss.BackFace.StencilFailOp = ConvertStencilOp(_dss.back_face.stencil_fail_op);
			dss.BackFace.StencilFunc = ConvertComparisonFunc(_dss.back_face.stencil_func);
			dss.BackFace.StencilPassOp = ConvertStencilOp(_dss.back_face.stencil_pass_op);
			return dss;
		}
		D3D11_BLEND_DESC ConvertBlendDesc(GfxBlendStateDesc _bd)
		{
			D3D11_BLEND_DESC bd{};
			bd.AlphaToCoverageEnable = _bd.alpha_to_coverage_enable;
			bd.IndependentBlendEnable = _bd.independent_blend_enable;
			for (int32 i = 0; i < 8; ++i)
			{
				bd.RenderTarget[i].BlendEnable = _bd.render_target[i].blend_enable;
				bd.RenderTarget[i].SrcBlend = ConvertBlend(_bd.render_target[i].src_blend);
				bd.RenderTarget[i].DestBlend = ConvertBlend(_bd.render_target[i].dest_blend);
				bd.RenderTarget[i].BlendOp = ConvertBlendOp(_bd.render_target[i].blend_op);
				bd.RenderTarget[i].SrcBlendAlpha = ConvertAlphaBlend(_bd.render_target[i].src_blend_alpha);
				bd.RenderTarget[i].DestBlendAlpha = ConvertAlphaBlend(_bd.render_target[i].dest_blend_alpha);
				bd.RenderTarget[i].BlendOpAlpha = ConvertBlendOp(_bd.render_target[i].blend_op_alpha);
				bd.RenderTarget[i].RenderTargetWriteMask = ParseColorWriteMask(_bd.render_target[i].render_target_write_mask);
			}
			return bd;
		}
		D3D11_SAMPLER_DESC ConvertSamplerDesc(GfxSamplerDesc const& desc)
		{
			D3D11_SAMPLER_DESC d3d11_desc{};
			d3d11_desc.Filter = ConvertFilter(desc.filter);
			d3d11_desc.AddressU = ConvertTextureAddressMode(desc.addressU);
			d3d11_desc.AddressV = ConvertTextureAddressMode(desc.addressV);
			d3d11_desc.AddressW = ConvertTextureAddressMode(desc.addressW);
			memcpy(d3d11_desc.BorderColor, desc.border_color, sizeof(float[4]));
			d3d11_desc.ComparisonFunc = ConvertComparisonFunc(desc.comparison_func);
			d3d11_desc.MinLOD = desc.min_lod;
			d3d11_desc.MaxLOD = desc.max_lod;
			d3d11_desc.MaxAnisotropy = desc.max_anisotropy;
			d3d11_desc.MipLODBias = desc.mip_lod_bias;
			return d3d11_desc;
		}
	}

	GfxRasterizerState::GfxRasterizerState(GfxDevice* gfx, GfxRasterizerStateDesc const& desc)
	{
		D3D11_RASTERIZER_DESC d3d11_desc = ConvertRasterizerDesc(desc);
		GFX_CHECK_HR(gfx->GetDevice()->CreateRasterizerState(&d3d11_desc, rasterizer_state.GetAddressOf()));
	}

	GfxDepthStencilState::GfxDepthStencilState(GfxDevice* gfx, GfxDepthStencilStateDesc const& desc)
	{
		D3D11_DEPTH_STENCIL_DESC d3d11_desc = ConvertDepthStencilDesc(desc);
		GFX_CHECK_HR(gfx->GetDevice()->CreateDepthStencilState(&d3d11_desc, depth_stencil_state.GetAddressOf()));
	}

	GfxBlendState::GfxBlendState(GfxDevice* gfx, GfxBlendStateDesc const& desc)
	{
		D3D11_BLEND_DESC d3d11_desc = ConvertBlendDesc(desc);
		gfx->GetDevice()->CreateBlendState(&d3d11_desc, blend_state.GetAddressOf());
	}

	GfxSampler::GfxSampler(GfxDevice* gfx, GfxSamplerDesc const& desc)
	{
		D3D11_SAMPLER_DESC d3d11_desc = ConvertSamplerDesc(desc);
		GFX_CHECK_HR(gfx->GetDevice()->CreateSamplerState(&d3d11_desc, sampler.GetAddressOf()));
	}
}
