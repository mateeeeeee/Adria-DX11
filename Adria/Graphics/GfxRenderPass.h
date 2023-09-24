#pragma once
#include <optional>
#include <vector>
#include <unordered_map>
#include <d3d11.h>
#include "GfxFormat.h"

namespace adria
{
	using GfxColorDescriptor = ID3D11RenderTargetView*;
	using GfxDepthDescriptor = ID3D11DepthStencilView*;

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
			GfxClearColor(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f)
				: color{ r, g, b, a }
			{
			}
			GfxClearColor(const float(&_color)[4])
				: color{ _color[0], _color[1], _color[2], _color[3] }
			{
			}
			GfxClearColor(GfxClearColor const& other)
				: color{ other.color[0], other.color[1], other.color[2], other.color[3] }
			{
			}

			bool operator==(GfxClearColor const& other) const
			{
				return memcmp(color, other.color, sizeof(color)) == 0;
			}

			float color[4];
		};
		struct GfxClearDepthStencil
		{
			GfxClearDepthStencil(float depth = 0.0f, uint8 stencil = 1)
				: depth(depth), stencil(stencil)
			{}
			float depth;
			uint8 stencil;
		};

		GfxClearValue() : active_member(GfxActiveMember::None), depth_stencil{} {}

		GfxClearValue(float r, float g, float b, float a)
			: active_member(GfxActiveMember::Color), color(r, g, b, a)
		{
		}

		GfxClearValue(float(&_color)[4])
			: active_member(GfxActiveMember::Color), color{ _color }
		{
		}

		GfxClearValue(GfxClearColor const& color)
			: active_member(GfxActiveMember::Color), color(color)
		{}

		GfxClearValue(float depth, uint8 stencil)
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

		bool operator==(GfxClearValue const& other) const
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
	enum class GfxLoadAccessOp : uint8
	{
		Load,
		Clear,
		DontCare
	};

	struct GfxColorAttachmentDesc
	{
		GfxColorDescriptor view;
		GfxLoadAccessOp load_op;
		GfxClearValue clear_color;
	};

	struct GfxDepthAttachmentDesc
	{
		GfxDepthDescriptor view;
		GfxLoadAccessOp load_op;
		GfxClearValue clear_depth;
	};

	struct GfxRenderPassDesc
	{
		std::vector<GfxColorAttachmentDesc> rtv_attachments;
		std::optional<GfxDepthAttachmentDesc> dsv_attachment;
		uint32 width, height;
	};

	class GfxRenderPass
	{
	public:
		GfxRenderPass() = default;
		explicit GfxRenderPass(GfxRenderPassDesc const& desc);
		void Begin(ID3D11DeviceContext* context);
		void End(ID3D11DeviceContext* context);

	private:
		uint32 width, height;
		std::vector<GfxColorDescriptor> render_targets;
		std::unordered_map<uint64, GfxClearValue> clear_values;
		GfxDepthDescriptor depth_target = nullptr;
		bool depth_clear = false;
		GfxClearValue depth_clear_value;
	};

}

