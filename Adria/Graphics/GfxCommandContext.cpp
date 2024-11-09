#include "GfxCommandContext.h"
#include "GfxQuery.h"
#include "GfxBuffer.h"
#include "GfxTexture.h"
#include "GfxInputLayout.h"
#include "GfxRenderPass.h"
#include "GfxShaderProgram.h"
#include "Utilities/StringUtil.h"

namespace adria
{
	namespace
	{
		const GfxShaderResourceRO NULL_SRVS[16] = { nullptr };
		const GfxShaderResourceRW NULL_UAVS[16] = { nullptr };
		constexpr D3D_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(GfxPrimitiveTopology topology)
		{
			switch (topology)
			{
			case GfxPrimitiveTopology::PointList:
				return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			case GfxPrimitiveTopology::LineList:
				return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			case GfxPrimitiveTopology::LineStrip:
				return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			case GfxPrimitiveTopology::TriangleList:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case GfxPrimitiveTopology::TriangleStrip:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			default:
				if (topology >= GfxPrimitiveTopology::PatchList1 && topology <= GfxPrimitiveTopology::PatchList32)
					return D3D_PRIMITIVE_TOPOLOGY(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + ((Uint32)topology - (Uint32)GfxPrimitiveTopology::PatchList1));
				else return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
			}
		}
	}

	void GfxCommandContext::Begin()
	{
		current_vs = nullptr;
		current_ps = nullptr;
		current_hs = nullptr;
		current_ds = nullptr;
		current_gs = nullptr;
		current_cs = nullptr;

		current_blend_state = nullptr;
		current_rasterizer_state = nullptr;
		current_depth_state = nullptr;
		current_topology = GfxPrimitiveTopology::Undefined;

		current_render_pass = nullptr;
		current_input_layout = nullptr;
	}

	void GfxCommandContext::End()
	{
		//command_context->ClearState();
		++frame_count;
	}

	void GfxCommandContext::Flush()
	{
		command_context->Flush();
	}

	void GfxCommandContext::WaitForGPU()
	{
		Flush();
		GfxQuery query(gfx, QueryType::Event);
		EndQuery(&query);
		Bool32 result = false;
		while (!GetQueryData(&query, &result, sizeof(result)));
		ADRIA_ASSERT(result != 0);
	}

	void GfxCommandContext::Draw(Uint32 vertex_count, Uint32 instance_count /*= 1*/, Uint32 start_vertex_location /*= 0*/, Uint32 start_instance_location /*= 0*/)
	{
		if(instance_count == 1) command_context->Draw(vertex_count, start_vertex_location);
		else  command_context->DrawInstanced(vertex_count, instance_count, start_vertex_location, start_instance_location);
	}

	void GfxCommandContext::DrawIndexed(Uint32 index_count, Uint32 instance_count /*= 1*/, Uint32 index_offset /*= 0*/, Uint32 base_vertex_location /*= 0*/, Uint32 start_instance_location /*= 0*/)
	{
		if (instance_count == 1) command_context->DrawIndexed(index_count, index_offset, base_vertex_location);
		else  command_context->DrawIndexedInstanced(index_count, instance_count, index_offset, base_vertex_location, start_instance_location);
	}

	void GfxCommandContext::Dispatch(Uint32 group_count_x, Uint32 group_count_y, Uint32 group_count_z /*= 1*/)
	{
		command_context->Dispatch(group_count_x, group_count_y, group_count_z);
	}

	void GfxCommandContext::DrawIndirect(GfxBuffer const& buffer, Uint32 offset)
	{
		command_context->DrawInstancedIndirect(buffer.GetNative(), offset);
	}

	void GfxCommandContext::DrawIndexedIndirect(GfxBuffer const& buffer, Uint32 offset)
	{
		command_context->DrawIndexedInstancedIndirect(buffer.GetNative(), offset);
	}

	void GfxCommandContext::DispatchIndirect(GfxBuffer const& buffer, Uint32 offset)
	{
		command_context->DispatchIndirect(buffer.GetNative(), offset);
	}

	void GfxCommandContext::CopyBuffer(GfxBuffer& dst, GfxBuffer const& src)
	{
		command_context->CopyResource(dst.GetNative(), src.GetNative());
	}

	void GfxCommandContext::CopyBuffer(GfxBuffer& dst, Uint32 dst_offset, GfxBuffer const& src, Uint32 src_offset, Uint32 size)
	{
		D3D11_BOX box{ .left = src_offset, .right = src_offset + size };
		command_context->CopySubresourceRegion(dst.GetNative(), 0, dst_offset, 0, 0, src.GetNative(), 0, &box);
	}

	void GfxCommandContext::CopyTexture(GfxTexture& dst, GfxTexture const& src)
	{
		command_context->CopyResource(dst.GetNative(), src.GetNative());
	}

	void GfxCommandContext::CopyTexture(GfxTexture& dst, Uint32 dst_mip, Uint32 dst_array, GfxTexture const& src, Uint32 src_mip, Uint32 src_array)
	{
		command_context->CopySubresourceRegion(dst.GetNative(), dst_mip + dst.GetDesc().mip_levels * dst_array, 0, 0, 0, src.GetNative(),
			src_mip + src.GetDesc().mip_levels * src_array, nullptr);
	}

	void GfxCommandContext::BeginRenderPass(GfxRenderPassDesc const& desc)
	{
		std::vector<GfxRenderTarget> render_targets;
		std::unordered_map<Uint64, GfxClearValue> clear_values;
		GfxDepthTarget depth_target = nullptr;
		Bool depth_clear = false;
		GfxClearValue depth_clear_value;

		for (Uint32 i = 0; i < desc.rtv_attachments.size(); ++i)
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
		SetViewport(0, 0, desc.width, desc.height);
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

	void GfxCommandContext::SetIndexBuffer(GfxBuffer* index_buffer, Uint32 offset)
	{
		if (index_buffer) command_context->IASetIndexBuffer(index_buffer->GetNative(), ConvertGfxFormat(index_buffer->GetDesc().format), offset);
		else command_context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	}

	void GfxCommandContext::SetVertexBuffer(GfxBuffer* vertex_buffer, Uint32 slot /*= 0*/)
	{
		GfxBuffer* vertex_buffers[1] = { vertex_buffer };
		SetVertexBuffers(vertex_buffers, slot);
	}

	void GfxCommandContext::SetVertexBuffers(std::span<GfxBuffer*> vertex_buffers, Uint32 start_slot /*= 0*/)
	{
		std::vector<ID3D11Buffer*> d3d11_buffers(vertex_buffers.size());
		std::vector<Uint32> strides(vertex_buffers.size());
		std::vector<Uint32> offsets(vertex_buffers.size());
		for (Uint32 i = 0; i < vertex_buffers.size(); ++i)
		{
			d3d11_buffers[i] = vertex_buffers[i] ? vertex_buffers[i]->GetNative() : nullptr;
			strides[i] = vertex_buffers[i] ? vertex_buffers[i]->GetDesc().stride : 0;
			offsets[i] = 0;
		}
		command_context->IASetVertexBuffers(start_slot, (Uint32)vertex_buffers.size(), d3d11_buffers.data(), strides.data(), offsets.data());
	}

	void GfxCommandContext::SetViewport(Uint32 x, Uint32 y, Uint32 width, Uint32 height)
	{
		D3D11_VIEWPORT vp{};
		vp.MinDepth = 0.0f; 
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = (Float)x;
		vp.TopLeftY = (Float)y;
		vp.Width = (Float)width;
		vp.Height = (Float)height;
		command_context->RSSetViewports(1, &vp);
	}

	void GfxCommandContext::SetScissorRect(Uint32 x, Uint32 y, Uint32 width, Uint32 height)
	{
		D3D11_RECT rect{};
		rect.left = x;
		rect.right = x + width;
		rect.top = y;
		rect.bottom = y + height;
		command_context->RSSetScissorRects(1, &rect);
	}

	void GfxCommandContext::ClearReadWriteDescriptorFloat(GfxShaderResourceRW descriptor, const Float v[4])
	{
		command_context->ClearUnorderedAccessViewFloat(descriptor, v);
	}

	void GfxCommandContext::ClearReadWriteDescriptorUint(GfxShaderResourceRW descriptor, const Uint32 v[4])
	{
		command_context->ClearUnorderedAccessViewUint(descriptor, v);
	}

	void GfxCommandContext::ClearRenderTarget(GfxRenderTarget rtv, Float const* clear_color)
	{
		command_context->ClearRenderTargetView(rtv, clear_color);
	}

	void GfxCommandContext::ClearDepth(GfxDepthTarget dsv, Float depth /*= 1.0f*/, Uint8 stencil /*= 0*/, Bool clear_stencil /*= false*/)
	{
		Uint32 flags = D3D11_CLEAR_DEPTH;
		if (clear_stencil) flags |= D3D11_CLEAR_STENCIL;
		command_context->ClearDepthStencilView(dsv, flags, depth, stencil);
	}

	void GfxCommandContext::SetInputLayout(GfxInputLayout* il)
	{
		if (current_input_layout != il)
		{
			current_input_layout = il;
			if (il) command_context->IASetInputLayout(*il);
			else command_context->IASetInputLayout(nullptr);
		}
	}

	void GfxCommandContext::SetDepthStencilState(GfxDepthStencilState* dss, Uint32 stencil_ref)
	{
		if (current_depth_state != dss)
		{
			current_depth_state = dss;
			if(dss) command_context->OMSetDepthStencilState(*dss, stencil_ref);
			else command_context->OMSetDepthStencilState(nullptr, stencil_ref);
		}
	}

	void GfxCommandContext::SetRasterizerState(GfxRasterizerState* rs)
	{
		if (current_rasterizer_state != rs)
		{
			current_rasterizer_state = rs;
			if (rs) command_context->RSSetState(*rs);
			else command_context->RSSetState(nullptr);
		}
	}

	void GfxCommandContext::SetBlendState(GfxBlendState* bs, Float* blend_factors, Uint32 mask /*= 0xffffffff*/)
	{
		if (current_blend_state != bs)
		{
			current_blend_state = bs;
			if (bs) command_context->OMSetBlendState(*bs, blend_factors, mask);
			else command_context->OMSetBlendState(nullptr, blend_factors, mask);
		}
	}

	void GfxCommandContext::CopyStructureCount(GfxBuffer* dst_buffer, Uint32 dst_buffer_offset, GfxShaderResourceRW src_view)
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

	GfxMappedSubresource GfxCommandContext::MapTexture(GfxTexture* texture, GfxMapType map_type, Uint32 subresource /*= 0*/)
	{
		D3D11_MAPPED_SUBRESOURCE mapped_subresource{};
		GFX_CHECK_HR(command_context->Map(texture->GetNative(), subresource, ConvertMapType(map_type), 0, &mapped_subresource));
		return GfxMappedSubresource
		{ .p_data = mapped_subresource.pData,
		 .row_pitch = mapped_subresource.RowPitch,
		 .depth_pitch = mapped_subresource.DepthPitch };
	}

	void GfxCommandContext::UnmapTexture(GfxTexture* texture, Uint32 subresource /*= 0*/)
	{
		command_context->Unmap(texture->GetNative(), 0);
	}

	void GfxCommandContext::UpdateBuffer(GfxBuffer* buffer, void const* data, Uint32 data_size)
	{
		GfxBufferDesc desc = buffer->GetDesc();

		if (desc.resource_usage == GfxResourceUsage::Dynamic)
		{
			GfxMappedSubresource mapped_buffer = MapBuffer(buffer, GfxMapType::WriteDiscard);
			memcpy(mapped_buffer.p_data, data, data_size);
			UnmapBuffer(buffer);
		}
		else command_context->UpdateSubresource(buffer->GetNative(), 0, nullptr, &data, data_size, 0);
	}

	void GfxCommandContext::SetVertexShader(GfxVertexShader* shader)
	{
		if (shader != current_vs)
		{
			current_vs = shader;
			command_context->VSSetShader(shader ? *shader : nullptr, nullptr, 0);
		}
	}

	void GfxCommandContext::SetPixelShader(GfxPixelShader* shader)
	{
		if (shader != current_ps)
		{
			current_ps = shader;
			command_context->PSSetShader(shader ? *shader : nullptr, nullptr, 0);
		}
	}

	void GfxCommandContext::SetHullShader(GfxHullShader* shader)
	{
		if (shader != current_hs)
		{
			current_hs = shader;
			command_context->HSSetShader(shader ? *shader : nullptr, nullptr, 0);
		}
	}

	void GfxCommandContext::SetDomainShader(GfxDomainShader* shader)
	{
		if (shader != current_ds)
		{
			current_ds = shader;
			command_context->DSSetShader(shader ? *shader : nullptr, nullptr, 0);
		}
	}

	void GfxCommandContext::SetGeometryShader(GfxGeometryShader* shader)
	{
		if (shader != current_gs)
		{
			current_gs = shader;
			command_context->GSSetShader(shader ? *shader : nullptr, nullptr, 0);
		}
	}

	void GfxCommandContext::SetComputeShader(GfxComputeShader* shader)
	{
		if (shader != current_cs)
		{
			current_cs = shader;
			command_context->CSSetShader(shader ? *shader : nullptr, nullptr, 0);
		}
	}

	void GfxCommandContext::SetConstantBuffer(GfxShaderStage stage, Uint32 slot, GfxBuffer* buffer)
	{
		GfxBuffer* buffers[] = { buffer };
		SetConstantBuffers(stage, slot, buffers);
	}

	void GfxCommandContext::SetConstantBuffers(GfxShaderStage stage, Uint32 start, std::span<GfxBuffer*> buffers)
	{
		std::vector<ID3D11Buffer*> d3d11_buffers(buffers.size());
		for (Uint32 i = 0; i < buffers.size(); ++i) d3d11_buffers[i] = buffers[i] ?  buffers[i]->GetNative() : nullptr;
		switch (stage)
		{
		case GfxShaderStage::VS:
			command_context->VSSetConstantBuffers(start, (Uint32)d3d11_buffers.size(), d3d11_buffers.data());
			break;
		case GfxShaderStage::PS:
			command_context->PSSetConstantBuffers(start, (Uint32)d3d11_buffers.size(), d3d11_buffers.data());
			break;
		case GfxShaderStage::HS:
			command_context->HSSetConstantBuffers(start, (Uint32)d3d11_buffers.size(), d3d11_buffers.data());
			break;
		case GfxShaderStage::DS:
			command_context->DSSetConstantBuffers(start, (Uint32)d3d11_buffers.size(), d3d11_buffers.data());
			break;
		case GfxShaderStage::GS:
			command_context->GSSetConstantBuffers(start, (Uint32)d3d11_buffers.size(), d3d11_buffers.data());
			break;
		case GfxShaderStage::CS:
			command_context->CSSetConstantBuffers(start, (Uint32)d3d11_buffers.size(), d3d11_buffers.data());
			break;
		}
	}

	void GfxCommandContext::SetSampler(GfxShaderStage stage, Uint32 start, GfxSampler* sampler)
	{
		GfxSampler* samplers[1] = { sampler };
		SetSamplers(stage, start, samplers);
	}

	void GfxCommandContext::SetSamplers(GfxShaderStage stage, Uint32 start, std::span<GfxSampler*> samplers)
	{
		std::vector<ID3D11SamplerState*> d3d11_samplers(samplers.size());
		for (Uint32 i = 0; i < samplers.size(); ++i) d3d11_samplers[i] = *samplers[i];
		switch (stage)
		{
		case GfxShaderStage::VS:
			command_context->VSSetSamplers(start, (Uint32)d3d11_samplers.size(), d3d11_samplers.data());
			break;
		case GfxShaderStage::PS:
			command_context->PSSetSamplers(start, (Uint32)d3d11_samplers.size(), d3d11_samplers.data());
			break;
		case GfxShaderStage::HS:
			command_context->HSSetSamplers(start, (Uint32)d3d11_samplers.size(), d3d11_samplers.data());
			break;
		case GfxShaderStage::DS:
			command_context->DSSetSamplers(start, (Uint32)d3d11_samplers.size(), d3d11_samplers.data());
			break;
		case GfxShaderStage::GS:
			command_context->GSSetSamplers(start, (Uint32)d3d11_samplers.size(), d3d11_samplers.data());
			break;
		case GfxShaderStage::CS:
			command_context->CSSetSamplers(start, (Uint32)d3d11_samplers.size(), d3d11_samplers.data());
			break;
		}
	}

	void GfxCommandContext::SetShaderResourceRO(GfxShaderStage stage, Uint32 slot, GfxShaderResourceRO descriptor)
	{
		GfxShaderResourceRO descriptors[] = { descriptor };
		SetShaderResourcesRO(stage, slot, descriptors);
	}

	void GfxCommandContext::SetShaderResourcesRO(GfxShaderStage stage, Uint32 start, std::span<GfxShaderResourceRO> descriptors)
	{
		switch (stage)
		{
		case GfxShaderStage::VS:
			command_context->VSSetShaderResources(start, (Uint32)descriptors.size(), descriptors.data());
			break;
		case GfxShaderStage::PS:
			command_context->PSSetShaderResources(start, (Uint32)descriptors.size(), descriptors.data());
			break;
		case GfxShaderStage::HS:
			command_context->HSSetShaderResources(start, (Uint32)descriptors.size(), descriptors.data());
			break;
		case GfxShaderStage::DS:
			command_context->DSSetShaderResources(start, (Uint32)descriptors.size(), descriptors.data());
			break;
		case GfxShaderStage::GS:
			command_context->GSSetShaderResources(start, (Uint32)descriptors.size(), descriptors.data());
			break;
		case GfxShaderStage::CS:
			command_context->CSSetShaderResources(start, (Uint32)descriptors.size(), descriptors.data());
			break;
		}
	}

	void GfxCommandContext::UnsetShaderResourcesRO(GfxShaderStage stage, Uint32 start, Uint32 count)
	{
		switch (stage)
		{
		case GfxShaderStage::VS:
			command_context->VSSetShaderResources(start, count, NULL_SRVS);
			break;
		case GfxShaderStage::PS:
			command_context->PSSetShaderResources(start, count, NULL_SRVS);
			break;
		case GfxShaderStage::HS:
			command_context->HSSetShaderResources(start, count, NULL_SRVS);
			break;
		case GfxShaderStage::DS:
			command_context->DSSetShaderResources(start, count, NULL_SRVS);
			break;
		case GfxShaderStage::GS:
			command_context->GSSetShaderResources(start, count, NULL_SRVS);
			break;
		case GfxShaderStage::CS:
			command_context->CSSetShaderResources(start, count, NULL_SRVS);
			break;
		}
	}

	void GfxCommandContext::SetShaderResourceRW(Uint32 slot, GfxShaderResourceRW descriptor)
	{
		GfxShaderResourceRW descriptors[] = { descriptor };
		SetShaderResourcesRW(slot, descriptors);
	}

	void GfxCommandContext::SetShaderResourcesRW(Uint32 start, std::span<GfxShaderResourceRW> descriptors)
	{
		command_context->CSSetUnorderedAccessViews(start, (Uint32)descriptors.size(), descriptors.data(), nullptr);
	}

	void GfxCommandContext::SetShaderResourcesRW(Uint32 start, std::span<GfxShaderResourceRW> descriptors, std::span<Uint32> initial_counts)
	{
		ADRIA_ASSERT(descriptors.size() == initial_counts.size());
		command_context->CSSetUnorderedAccessViews(start, (Uint32)descriptors.size(), descriptors.data(), initial_counts.data());
	}

	void GfxCommandContext::UnsetShaderResourcesRW(Uint32 start, Uint32 count)
	{
		command_context->CSSetUnorderedAccessViews(start, count, NULL_UAVS, nullptr);
	}


	void GfxCommandContext::SetRenderTarget(GfxRenderTarget rtv, GfxDepthTarget dsv /*= nullptr*/)
	{
		command_context->OMSetRenderTargets(1, &rtv, dsv);
	}

	void GfxCommandContext::SetRenderTargets(std::span<GfxRenderTarget> rtvs, GfxDepthTarget dsv /*= nullptr*/)
	{
		command_context->OMSetRenderTargets((Uint32)rtvs.size(), rtvs.data(), dsv);
	}

	void GfxCommandContext::SetRenderTargetsAndShaderResourcesRW(std::span<GfxRenderTarget> rtvs, GfxDepthTarget dsv, Uint32 start_slot, std::span<GfxShaderResourceRW> uavs, std::span<Uint32> initial_counts /*= {}*/)
	{
		command_context->OMSetRenderTargetsAndUnorderedAccessViews((Uint32)rtvs.size(), rtvs.data(), dsv, start_slot,
																   (Uint32)uavs.size(), uavs.data(), initial_counts.data());
	}

	void GfxCommandContext::GenerateMips(GfxShaderResourceRO srv)
	{
		command_context->GenerateMips(srv);
	}

	void GfxCommandContext::BeginQuery(GfxQuery* query)
	{
		command_context->Begin(*query);
	}

	void GfxCommandContext::EndQuery(GfxQuery* query)
	{
		command_context->End(*query);
	}

	Bool GfxCommandContext::GetQueryData(GfxQuery* query, void* data, Uint32 data_size)
	{
		return command_context->GetData(*query, data, data_size, 0) == S_OK;
	}

	void GfxCommandContext::BeginEvent(Char const* event_name)
	{
		annotation->BeginEvent(ToWideString(event_name).c_str());
	}

	void GfxCommandContext::EndEvent()
	{
		annotation->EndEvent();
	}

}