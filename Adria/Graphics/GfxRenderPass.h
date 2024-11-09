#pragma once
#include <optional>
#include <vector>
#include <unordered_map>
#include <d3d11.h>
#include "GfxFormat.h"
#include "GfxView.h"

namespace adria
{

	struct GfxClearValue
	{
		enum class GfxActiveMember
		{
			None,
			Color,
			DepthStencil
		};
		struct GfxClearColor
		{
			GfxClearColor(Float r = 0.0f, Float g = 0.0f, Float b = 0.0f, Float a = 0.0f)
				: color{ r, g, b, a }
			{
			}
			GfxClearColor(const Float(&_color)[4])
				: color{ _color[0], _color[1], _color[2], _color[3] }
			{
			}
			GfxClearColor(GfxClearColor const& other)
				: color{ other.color[0], other.color[1], other.color[2], other.color[3] }
			{
			}

			Bool operator==(GfxClearColor const& other) const
			{
				return memcmp(color, other.color, sizeof(color)) == 0;
			}

			Float color[4];
		};
		struct GfxClearDepthStencil
		{
			GfxClearDepthStencil(Float depth = 0.0f, Uint8 stencil = 1)
				: depth(depth), stencil(stencil)
			{}
			Float depth;
			Uint8 stencil;
		};

		GfxClearValue() : active_member(GfxActiveMember::None), depth_stencil{} {}

		GfxClearValue(Float r, Float g, Float b, Float a)
			: active_member(GfxActiveMember::Color), color(r, g, b, a)
		{
		}

		GfxClearValue(Float(&_color)[4])
			: active_member(GfxActiveMember::Color), color{ _color }
		{
		}

		GfxClearValue(GfxClearColor const& color)
			: active_member(GfxActiveMember::Color), color(color)
		{}

		GfxClearValue(Float depth, Uint8 stencil)
			: active_member(GfxActiveMember::DepthStencil), depth_stencil(depth, stencil)
		{}
		GfxClearValue(GfxClearDepthStencil const& depth_stencil)
			: active_member(GfxActiveMember::DepthStencil), depth_stencil(depth_stencil)
		{}

		GfxClearValue(GfxClearValue const& other)
			: active_member(other.active_member), color{}, format(other.format)
		{
			if (active_member == GfxActiveMember::Color) color = other.color;
			else if (active_member == GfxActiveMember::DepthStencil) depth_stencil = other.depth_stencil;
		}

		GfxClearValue& operator=(GfxClearValue const& other)
		{
			if (this == &other) return *this;
			active_member = other.active_member;
			format = other.format;
			if (active_member == GfxActiveMember::Color) color = other.color;
			else if (active_member == GfxActiveMember::DepthStencil) depth_stencil = other.depth_stencil;
			return *this;
		}

		Bool operator==(GfxClearValue const& other) const
		{
			if (active_member != other.active_member) return false;
			else if (active_member == GfxActiveMember::Color)
			{
				return color == other.color;
			}
			else return depth_stencil.depth == other.depth_stencil.depth
				&& depth_stencil.stencil == other.depth_stencil.stencil;
		}

		GfxActiveMember active_member;
		GfxFormat format = GfxFormat::UNKNOWN;
		union
		{
			GfxClearColor color;
			GfxClearDepthStencil depth_stencil;
		};
	};
	enum class GfxLoadAccessOp : Uint8
	{
		Load,
		Clear,
		DontCare
	};

	struct GfxColorAttachmentDesc
	{
		GfxRenderTarget view;
		GfxLoadAccessOp load_op;
		GfxClearValue clear_color;
	};

	struct GfxDepthAttachmentDesc
	{
		GfxDepthTarget view;
		GfxLoadAccessOp load_op;
		GfxClearValue clear_depth;
	};

	struct GfxRenderPassDesc
	{
		std::vector<GfxColorAttachmentDesc> rtv_attachments;
		std::optional<GfxDepthAttachmentDesc> dsv_attachment;
		Uint32 width, height;
	};
}

