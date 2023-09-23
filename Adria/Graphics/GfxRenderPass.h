#pragma once
#include <optional>
#include <vector>
#include <array>
#include <unordered_map>
#include <d3d11.h>
#include "Core/CoreTypes.h" 

namespace adria
{
	using RtvType = ID3D11RenderTargetView*;
	using DsvType = ID3D11DepthStencilView*;
	
	enum class ELoadOp
	{
		Load,
		Clear,
		DontCare
	};

	struct RtvAttachmentDesc
	{
		RtvType view;
		ELoadOp load_op;
		std::array<float,4> clear_color;
	};

	struct DsvAttachmentDesc
	{
		DsvType view;
		ELoadOp load_op;
		float clear_depth;
		uint8 clear_stencil;
	};

	struct RenderPassDesc
	{
		std::vector<RtvAttachmentDesc> rtv_attachments;
		std::optional<DsvAttachmentDesc> dsv_attachment;
		uint32 width, height;
	};

	class GfxRenderPass
	{
	public:
		GfxRenderPass() = default;
		GfxRenderPass(RenderPassDesc const& desc);
		void Begin(ID3D11DeviceContext* context);
		void End(ID3D11DeviceContext* context);

	private:
		uint32 width, height;
		std::vector<RtvType> render_targets;
		std::unordered_map<uint64, std::array<float,4>> clear_values;
		DsvType depth_target = nullptr;
		bool depth_clear = false;
		float clear_depth = 0.0f;
		uint8 clear_stencil = 0;
	};

}

