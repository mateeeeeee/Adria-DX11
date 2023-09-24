#include "GfxCommandContext.h"
#include "GfxQuery.h"
#include "GfxBuffer.h"
#include "GfxTexture.h"
#include "GfxRenderPass.h"
#include "GfxShaderProgram.h"
#include "Utilities/StringUtil.h"

namespace adria
{

	void GfxCommandContext::Draw(uint32 vertex_count, uint32 instance_count /*= 1*/, uint32 start_vertex_location /*= 0*/, uint32 start_instance_location /*= 0*/)
	{
		if(instance_count == 1) command_context->Draw(vertex_count, start_vertex_location);
		else  command_context->DrawInstanced(vertex_count, instance_count, start_vertex_location, start_instance_location);
	}

	void GfxCommandContext::DrawIndexed(uint32 index_count, uint32 instance_count /*= 1*/, uint32 index_offset /*= 0*/, uint32 base_vertex_location /*= 0*/, uint32 start_instance_location /*= 0*/)
	{
		if (instance_count == 1) command_context->DrawIndexed(index_count, index_offset, base_vertex_location);
		else  command_context->DrawIndexedInstanced(index_count, instance_count, index_offset, base_vertex_location, start_instance_location);
	}

	void GfxCommandContext::Dispatch(uint32 group_count_x, uint32 group_count_y, uint32 group_count_z /*= 1*/)
	{
		command_context->Dispatch(group_count_x, group_count_y, group_count_z);
	}

	void GfxCommandContext::DrawIndirect(GfxBuffer const& buffer, uint32 offset)
	{
		command_context->DrawInstancedIndirect(buffer.GetNative(), offset);
	}

	void GfxCommandContext::DrawIndexedIndirect(GfxBuffer const& buffer, uint32 offset)
	{
		command_context->DrawIndexedInstancedIndirect(buffer.GetNative(), offset);
	}

	void GfxCommandContext::DispatchIndirect(GfxBuffer const& buffer, uint32 offset)
	{
		command_context->DispatchIndirect(buffer.GetNative(), offset);
	}

	void GfxCommandContext::CopyBuffer(GfxBuffer& dst, GfxBuffer const& src)
	{
		command_context->CopyResource(dst.GetNative(), src.GetNative());
	}

	void GfxCommandContext::CopyBuffer(GfxBuffer& dst, uint64 dst_offset, GfxBuffer const& src, uint64 src_offset, uint64 size)
	{
		D3D11_BOX box{ .left = src_offset, .right = src_offset + size };
		command_context->CopySubresourceRegion(dst.GetNative(), 0, dst_offset, 0, 0, src.GetNative(), 0, &box);
	}

	void GfxCommandContext::CopyTexture(GfxTexture& dst, GfxTexture const& src)
	{
		command_context->CopyResource(dst.GetNative(), src.GetNative());
	}

	void GfxCommandContext::CopyTexture(GfxTexture& dst, uint32 dst_mip, uint32 dst_array, GfxTexture const& src, uint32 src_mip, uint32 src_array)
	{
		command_context->CopySubresourceRegion(dst.GetNative(), dst_mip + dst.GetDesc().mip_levels * dst_array, 0, 0, 0, src.GetNative(),
			src_mip + src.GetDesc().mip_levels * src_array, nullptr);
	}

	void GfxCommandContext::BeginRenderPass(GfxRenderPassDesc const& desc)
	{
		std::vector<GfxColorDescriptor> render_targets;
		std::unordered_map<uint64, GfxClearValue> clear_values;
		GfxDepthDescriptor depth_target = nullptr;
		bool depth_clear = false;
		GfxClearValue depth_clear_value;

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

		for (auto const& [index, clear_value] : clear_values)
		{
			ClearRenderTarget(render_targets[index], clear_value.color.color);
		}

		if (depth_clear)
		{
			ClearDepth(depth_target, depth_clear_value.depth_stencil.depth, depth_clear_value.depth_stencil.stencil, false);
		}
		SetRenderTargets(render_targets, depth_target);
		SetViewport(0.0f, 0.0f, desc.width, desc.height);
	}

	void GfxCommandContext::EndRenderPass()
	{
		command_context->OMSetRenderTargets(0, nullptr, nullptr);
	}

	void GfxCommandContext::SetTopology(GfxPrimitiveTopology topology)
	{
		if (current_topology != topology)
		{
			current_topology = topology;
			command_context->IASetPrimitiveTopology(ConvertPrimitiveTopology(current_topology));
		}
	}

	void GfxCommandContext::SetIndexBuffer(GfxBuffer* index_buffer, uint32 offset)
	{
		command_context->IASetIndexBuffer(index_buffer->GetNative(), ConvertGfxFormat(index_buffer->GetDesc().format), offset);
	}

	void GfxCommandContext::SetVertexBuffers(std::span<GfxBuffer*> vertex_buffer_views, uint32 start_slot /*= 0*/)
	{

	}

	void GfxCommandContext::SetViewport(uint32 x, uint32 y, uint32 width, uint32 height)
	{
		D3D11_VIEWPORT vp{};
		vp.MinDepth = 0.0f; 
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = x;
		vp.TopLeftY = y;
		vp.Width = width;
		vp.Height = height;
		command_context->RSSetViewports(1, &vp);
	}

	void GfxCommandContext::SetScissorRect(uint32 x, uint32 y, uint32 width, uint32 height)
	{
		D3D11_RECT rect{};
		rect.left = x;
		rect.right = x + width;
		rect.top = y;
		rect.bottom = y + height;
		command_context->RSSetScissorRects(1, &rect);
	}

	void GfxCommandContext::ClearReadWriteDescriptorFloat(GfxReadWriteDescriptor descriptor, const float v[4])
	{
		command_context->ClearUnorderedAccessViewFloat(descriptor, v);
	}

	void GfxCommandContext::ClearReadWriteDescriptorUint(GfxReadWriteDescriptor descriptor, const uint32 v[4])
	{
		command_context->ClearUnorderedAccessViewUint(descriptor, v);
	}

	void GfxCommandContext::ClearRenderTarget(GfxColorDescriptor rtv, float const* clear_color)
	{
		command_context->ClearRenderTargetView(rtv, clear_color);
	}

	void GfxCommandContext::ClearDepth(GfxDepthDescriptor dsv, float depth /*= 1.0f*/, uint8 stencil /*= 0*/, bool clear_stencil /*= false*/)
	{
		uint32 flags = D3D11_CLEAR_DEPTH;
		if (clear_stencil) flags |= D3D11_CLEAR_STENCIL;
		command_context->ClearDepthStencilView(dsv, flags, depth, stencil);
	}

	void GfxCommandContext::SetRenderTargets(std::span<GfxColorDescriptor> rtvs, GfxDepthDescriptor dsv /*= nullptr*/)
	{
		command_context->OMSetRenderTargets(rtvs.size(), rtvs.data(), dsv);
	}

	void GfxCommandContext::SetDepthStencilState(GfxDepthStencilState* dss, uint32 stencil_ref)
	{

	}

	void GfxCommandContext::SetRasterizerState(GfxRasterizerState* rs)
	{

	}

	void GfxCommandContext::SetBlendStateState(GfxBlendState* bs, float blend_factors[4], uint32 mask /*= 0xffffffff*/)
	{

	}

	void GfxCommandContext::CopyStructureCount(GfxBuffer* dst_buffer, uint32 dst_buffer_offset, GfxReadWriteDescriptor src_view)
	{
		command_context->CopyStructureCount(dst_buffer->GetNative(), dst_buffer_offset, src_view);
	}

	GfxMappedSubresource GfxCommandContext::MapBuffer(GfxBuffer* buffer, GfxMapType map_type)
	{
		D3D11_MAPPED_SUBRESOURCE mapped_subresource{};
		GFX_CHECK_HR(command_context->Map(buffer->GetNative(), 0, ConvertMapType(map_type), 0, &mapped_subresource));
		return GfxMappedSubresource
		{.p_data = mapped_subresource.pData,
		 .row_pitch = mapped_subresource.RowPitch,
		 .depth_pitch = mapped_subresource.DepthPitch };
	}

	void GfxCommandContext::UnmapBuffer(GfxBuffer* buffer)
	{
		command_context->Unmap(buffer->GetNative(), 0);
	}

	GfxMappedSubresource GfxCommandContext::MapTexture(GfxTexture* texture, GfxMapType map_type, uint32 subresource /*= 0*/)
	{
		D3D11_MAPPED_SUBRESOURCE mapped_subresource{};
		GFX_CHECK_HR(command_context->Map(texture->GetNative(), subresource, ConvertMapType(map_type), 0, &mapped_subresource));
		return GfxMappedSubresource
		{ .p_data = mapped_subresource.pData,
		 .row_pitch = mapped_subresource.RowPitch,
		 .depth_pitch = mapped_subresource.DepthPitch };
	}

	void GfxCommandContext::UnmapTexture(GfxTexture* texture, uint32 subresource /*= 0*/)
	{
		command_context->Unmap(texture->GetNative(), 0);
	}

	void GfxCommandContext::SetConstantBuffers(GfxShaderStage stage, uint32 start, std::span<GfxBuffer*> buffers)
	{
		std::vector<ID3D11Buffer*> d3d11_buffers(buffers.size());
		for (uint32 i = 0; i < buffers.size(); ++i) d3d11_buffers[i] = buffers[i]->GetNative();
		switch (stage)
		{
		case GfxShaderStage::VS:
			command_context->VSSetConstantBuffers(start, d3d11_buffers.size(), d3d11_buffers.data());
		case GfxShaderStage::PS:
			command_context->PSSetConstantBuffers(start, d3d11_buffers.size(), d3d11_buffers.data());
		case GfxShaderStage::HS:
			command_context->HSSetConstantBuffers(start, d3d11_buffers.size(), d3d11_buffers.data());
		case GfxShaderStage::DS:
			command_context->DSSetConstantBuffers(start, d3d11_buffers.size(), d3d11_buffers.data());
		case GfxShaderStage::GS:
			command_context->GSSetConstantBuffers(start, d3d11_buffers.size(), d3d11_buffers.data());
		case GfxShaderStage::CS:
			command_context->CSSetConstantBuffers(start, d3d11_buffers.size(), d3d11_buffers.data());
		}
	}

	void GfxCommandContext::SetVertexShader(GfxVertexShader* shader)
	{
		if (shader != current_vs)
		{
			current_vs = shader;
			shader->Bind(command_context);
		}
	}

	void GfxCommandContext::SetPixelShader(GfxPixelShader* shader)
	{
		if (shader != current_ps)
		{
			current_ps = shader;
			shader->Bind(command_context);
		}
	}

	void GfxCommandContext::SetHullShader(GfxHullShader* shader)
	{
		if (shader != current_hs)
		{
			current_hs = shader;
			shader->Bind(command_context);
		}
	}

	void GfxCommandContext::SetDomainShader(GfxDomainShader* shader)
	{
		if (shader != current_ds)
		{
			current_ds = shader;
			shader->Bind(command_context);
		}
	}

	void GfxCommandContext::SetGeometryShader(GfxGeometryShader* shader)
	{
		if (shader != current_gs)
		{
			current_gs = shader;
			shader->Bind(command_context);
		}
	}

	void GfxCommandContext::SetComputeShader(GfxComputeShader* shader)
	{
		if (shader != current_cs)
		{
			current_cs = shader;
			shader->Bind(command_context);
		}
	}

	void GfxCommandContext::SetReadOnlyDescriptors(GfxShaderStage stage, uint32 start, std::span<GfxReadOnlyDescriptor> descriptors)
	{
		switch (stage)
		{
		case GfxShaderStage::VS:
			command_context->VSSetShaderResources(start, descriptors.size(), descriptors.data());
		case GfxShaderStage::PS:
			command_context->PSSetShaderResources(start, descriptors.size(), descriptors.data());
		case GfxShaderStage::HS:
			command_context->HSSetShaderResources(start, descriptors.size(), descriptors.data());
		case GfxShaderStage::DS:
			command_context->DSSetShaderResources(start, descriptors.size(), descriptors.data());
		case GfxShaderStage::GS:
			command_context->GSSetShaderResources(start, descriptors.size(), descriptors.data());
		case GfxShaderStage::CS:
			command_context->CSSetShaderResources(start, descriptors.size(), descriptors.data());
		}
	}

	void GfxCommandContext::SetReadWriteDescriptors(uint32 start, std::span<GfxReadWriteDescriptor> descriptors)
	{
		command_context->CSSetUnorderedAccessViews(start, descriptors.size(), descriptors.data(), nullptr);
	}

	void GfxCommandContext::GenerateMips(GfxReadOnlyDescriptor srv)
	{
		command_context->GenerateMips(srv);
	}

	void GfxCommandContext::Begin(GfxQuery& query)
	{
		command_context->Begin(query);
	}

	void GfxCommandContext::End(GfxQuery& query)
	{
		command_context->End(query);
	}

	void GfxCommandContext::GetData(GfxQuery& query, void* data, uint32 data_size)
	{
		command_context->GetData(query, data, data_size, 0);
	}

	void GfxCommandContext::BeginEvent(char const* event_name)
	{
		annot->BeginEvent(ToWideString(event_name).c_str());
	}

	void GfxCommandContext::EndEvent()
	{
		annot->EndEvent();
	}

}