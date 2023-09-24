#include "GfxRenderPass.h"

namespace adria
{

	GfxRenderPass::GfxRenderPass(GfxRenderPassDesc const& desc) : width(desc.width), height(desc.height)
	{
		for (uint32 i = 0; i < desc.rtv_attachments.size(); ++i)
		{
			GfxColorAttachmentDesc const& attachment = desc.rtv_attachments[i];
			render_targets.push_back(attachment.view);
			if (attachment.load_op == GfxLoadAccessOp::Clear) clear_values[i] = attachment.clear_color;
		}

		if (desc.dsv_attachment)
		{
			GfxDepthAttachmentDesc const& dsv = desc.dsv_attachment.value();
			depth_target = dsv.view;
			if (dsv.load_op == GfxLoadAccessOp::Clear)
			{
				depth_clear = true;
				depth_clear_value = dsv.clear_depth;
			}
		}
	}

	void GfxRenderPass::Begin(ID3D11DeviceContext* context)
	{
		for (auto const& [index, clear_value] : clear_values)
		{
			context->ClearRenderTargetView(render_targets[index], clear_value.color.color);
		}

		if (depth_clear)
		{
			context->ClearDepthStencilView(depth_target, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
				depth_clear_value.depth_stencil.depth, depth_clear_value.depth_stencil.stencil);
		}
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

