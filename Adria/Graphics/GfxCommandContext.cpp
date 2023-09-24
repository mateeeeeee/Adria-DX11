#include "GfxCommandContext.h"

namespace adria
{

	void GfxCommandContext::Draw(uint32 vertex_count, uint32 instance_count /*= 1*/, uint32 start_vertex_location /*= 0*/, uint32 start_instance_location /*= 0*/)
	{

	}

	void GfxCommandContext::DrawIndexed(uint32 index_count, uint32 instance_count /*= 1*/, uint32 index_offset /*= 0*/, uint32 base_vertex_location /*= 0*/, uint32 start_instance_location /*= 0*/)
	{

	}

	void GfxCommandContext::Dispatch(uint32 group_count_x, uint32 group_count_y, uint32 group_count_z /*= 1*/)
	{

	}

	void GfxCommandContext::DrawIndirect(GfxBuffer const& buffer, uint32 offset)
	{

	}

	void GfxCommandContext::DrawIndexedIndirect(GfxBuffer const& buffer, uint32 offset)
	{

	}

	void GfxCommandContext::DispatchIndirect(GfxBuffer const& buffer, uint32 offset)
	{

	}

	void GfxCommandContext::CopyBuffer(GfxBuffer& dst, GfxBuffer const& src)
	{

	}

	void GfxCommandContext::CopyTexture(GfxTexture& dst, GfxTexture const& src)
	{

	}

	void GfxCommandContext::BeginRenderPass(GfxRenderPassDesc const& render_pass_desc)
	{

	}

	void GfxCommandContext::EndRenderPass()
	{

	}

	void GfxCommandContext::SetStencilReference(uint8 stencil)
	{

	}

	void GfxCommandContext::SetBlendFactor(float const* blend_factor)
	{

	}

	void GfxCommandContext::SetTopology(GfxPrimitiveTopology topology)
	{

	}

	void GfxCommandContext::SetIndexBuffer(GfxBuffer* index_buffer_view)
	{

	}

	void GfxCommandContext::SetVertexBuffers(std::span<GfxBuffer*> vertex_buffer_views, uint32 start_slot /*= 0*/)
	{

	}

	void GfxCommandContext::SetViewport(uint32 x, uint32 y, uint32 width, uint32 height)
	{

	}

	void GfxCommandContext::SetScissorRect(uint32 x, uint32 y, uint32 width, uint32 height)
	{

	}

	void GfxCommandContext::ClearReadWriteDescriptorFloat(GfxReadWriteDescriptor descriptor, const float v[4])
	{

	}

	void GfxCommandContext::ClearReadWriteDescriptorUint(GfxReadWriteDescriptor descriptor, const uint32 v[4])
	{

	}

	void GfxCommandContext::ClearRenderTarget(GfxColorDescriptor rtv, float const* clear_color)
	{

	}

	void GfxCommandContext::ClearDepth(GfxDepthDescriptor dsv, float depth /*= 1.0f*/, uint8 stencil /*= 0*/, bool clear_stencil /*= false*/)
	{

	}

	void GfxCommandContext::SetRenderTargets(std::span<GfxColorDescriptor> rtvs, GfxDepthDescriptor const* dsv /*= nullptr*/, bool single_rt /*= false*/)
	{

	}

	void GfxCommandContext::CopyStructureCount(GfxBuffer* dst_buffer, uint32 dst_buffer_offset, GfxReadWriteDescriptor src_view)
	{

	}

	GfxMappedSubresource GfxCommandContext::MapBuffer(GfxBuffer* buffer, GfxMapType map_type)
	{
		return GfxMappedSubresource{};
	}

	void GfxCommandContext::UnmapBuffer(GfxBuffer* buffer)
	{

	}

	GfxMappedSubresource GfxCommandContext::MapTexture(GfxTexture* texture, GfxMapType map_type, uint32 subresource /*= 0*/)
	{
		return GfxMappedSubresource{};
	}

	void GfxCommandContext::UnmapTexture(GfxTexture* texture, uint32 subresource /*= 0*/)
	{

	}

	void GfxCommandContext::SetConstantBuffers(GfxShaderStage stage, uint32 start, std::span<GfxBuffer*> buffers)
	{

	}

	void GfxCommandContext::SetVertexShader(GfxVertexShader* shader)
	{

	}

	void GfxCommandContext::SetPixelShader(GfxPixelShader* shader)
	{

	}

	void GfxCommandContext::SetHullShader(GfxHullShader* shader)
	{

	}

	void GfxCommandContext::SetDomainShader(GfxDomainShader* shader)
	{

	}

	void GfxCommandContext::SetGeometryShader(GfxGeometryShader* shader)
	{

	}

	void GfxCommandContext::SetComputeShader(GfxComputeShader* shader)
	{

	}

	void GfxCommandContext::SetReadOnlyDescriptors(GfxShaderStage stage, uint32 start, std::span<GfxReadOnlyDescriptor> descriptors)
	{

	}

	void GfxCommandContext::SetReadWriteDescriptors(GfxShaderStage stage, uint32 start, std::span<GfxReadWriteDescriptor> descriptors)
	{

	}

	void GfxCommandContext::GenerateMips(GfxReadOnlyDescriptor srv)
	{

	}

	void GfxCommandContext::CopyTexture(GfxTexture& dst, uint32 dst_mip, uint32 dst_array, GfxTexture const& src, uint32 src_mip, uint32 src_array)
	{

	}

	void GfxCommandContext::CopyBuffer(GfxBuffer& dst, uint64 dst_offset, GfxBuffer const& src, uint64 src_offset, uint64 size)
	{

	}


	void GfxCommandContext::Begin(GfxQuery& query)
	{

	}

	void GfxCommandContext::End(GfxQuery& query)
	{

	}

	void GfxCommandContext::GetData(GfxQuery& query, void* data, uint32 data_size)
	{

	}

	void GfxCommandContext::BeginEvent(char const* event_name)
	{

	}

	void GfxCommandContext::EndEvent()
	{

	}

}