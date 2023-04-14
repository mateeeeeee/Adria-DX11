#include "GfxRenderPass.h"

namespace adria
{

	GfxRenderPass::GfxRenderPass(RenderPassDesc const& desc) : width(desc.width), height(desc.height)
	{
		for (uint32 i = 0; i < desc.rtv_attachments.size(); ++i)
		{
			auto const& attachment = desc.rtv_attachments[i];

			render_targets.push_back(attachment.view);

			if (attachment.load_op == ELoadOp::Clear) clear_values[i] = attachment.clear_color;
		}

		if (desc.dsv_attachment.has_value())
		{
			auto const& dsv = desc.dsv_attachment.value();

			depth_target = dsv.view;

			if (dsv.load_op == ELoadOp::Clear)
			{
				depth_clear = true;
				clear_depth = dsv.clear_depth;
				clear_stencil = dsv.clear_stencil;
			}
		}
	}

	void GfxRenderPass::Begin(ID3D11DeviceContext* context)
	{
		for (auto const& [index, clear_value] : clear_values)
			context->ClearRenderTargetView(render_targets[index], clear_value.data());

		if (depth_clear)
			context->ClearDepthStencilView(depth_target, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
				clear_depth, clear_stencil);

		context->OMSetRenderTargets(static_cast<uint32>(render_targets.size()), render_targets.data(), depth_target);

		D3D11_VIEWPORT vp{};
		vp.TopLeftX = vp.TopLeftY = 0.0f;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.Width = static_cast<float>(width);
		vp.Height = static_cast<float>(height);

		context->RSSetViewports(1, &vp);
	}

	void GfxRenderPass::End(ID3D11DeviceContext* context)
	{
		context->OMSetRenderTargets(0, nullptr, nullptr);
	}

}

