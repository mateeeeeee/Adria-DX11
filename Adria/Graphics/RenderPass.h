#pragma once
#include <variant>
#include <vector>
#include <d3d11.h>
#include <wrl.h>
#include "../Core/Definitions.h" 

namespace adria
{
	
	using rtv_t = ID3D11RenderTargetView*;
	using dsv_t = ID3D11DepthStencilView*;
	
	enum class LoadOp
	{
		eLoad,
		eClear,
		eDontCare
	};

	struct rtv_attachment_desc_t
	{
		rtv_t view;
		LoadOp load_op;
		std::array<f32,4> clear_color;
	};

	struct dsv_attachment_desc_t
	{
		dsv_t view;
		LoadOp load_op;
		f32 clear_depth;
		u8 clear_stencil;
	};

	
	struct render_pass_desc_t
	{
		std::vector<rtv_attachment_desc_t> rtv_attachments;
		std::optional<dsv_attachment_desc_t> dsv_attachment;
		u32 width, height;
	};


	
	class RenderPass
	{
		
	public:
		RenderPass() = default;

		RenderPass(render_pass_desc_t const& desc) : width(desc.width), height(desc.height)
		{
			for (u32 i = 0; i < desc.rtv_attachments.size(); ++i) 
			{
				auto const& attachment = desc.rtv_attachments[i];

				render_targets.push_back(attachment.view);

				if (attachment.load_op == LoadOp::eClear) clear_values[i] = attachment.clear_color;
			}

			if (desc.dsv_attachment.has_value())
			{
				auto const& dsv = desc.dsv_attachment.value();

				depth_target = dsv.view;

				if (dsv.load_op == LoadOp::eClear)
				{
					depth_clear = true;
					clear_depth = dsv.clear_depth;
					clear_stencil = dsv.clear_stencil;
				}
			}
		}

		void Begin(ID3D11DeviceContext* context)
		{
			for (auto const& [index, clear_value] : clear_values)
				context->ClearRenderTargetView(render_targets[index], clear_value.data());

			if(depth_clear)
				context->ClearDepthStencilView(depth_target, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
					clear_depth, clear_stencil);

			context->OMSetRenderTargets(static_cast<u32>(render_targets.size()), render_targets.data(), depth_target);

			D3D11_VIEWPORT vp{};
			vp.TopLeftX = vp.TopLeftY = 0.0f;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.Width = static_cast<float>(width);
			vp.Height = static_cast<float>(height);

			context->RSSetViewports(1, &vp);
		}

		void End(ID3D11DeviceContext* context)
		{
			context->OMSetRenderTargets(0, nullptr, nullptr);
		}

	private:
		u32 width, height;
		std::vector<rtv_t> render_targets;
		std::unordered_map<u64, std::array<f32,4>> clear_values;
		dsv_t depth_target = nullptr;
		bool depth_clear = false;
		f32 clear_depth = 0.0f;
		u8 clear_stencil = 0;
		
	};

}

