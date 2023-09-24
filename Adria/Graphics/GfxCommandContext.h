#pragma once
#include <span>
#include "GfxStates.h"
#include "GfxDescriptor.h"
#include "GfxResourceCommon.h"
#include "GfxShader.h"

namespace adria
{
	class GfxDevice;
	class GfxVertexShader;
	class GfxPixelShader;
	class GfxHullShader;
	class GfxDomainShader;
	class GfxGeometryShader;
	class GfxComputeShader;

	class GfxBuffer;
	class GfxTexture;
	class GfxQuery;

	struct GfxRenderPassDesc;
	struct GfxRasterizerState;
	struct GfxBlendState;
	struct GfxDepthStencilState;
	struct GfxInputLayout;

	class GfxCommandContext
	{
	public:
		explicit GfxCommandContext(GfxDevice* gfx) : gfx(gfx) 
		{
		
		}

		void Draw(uint32 vertex_count, uint32 instance_count = 1, uint32 start_vertex_location = 0, uint32 start_instance_location = 0);
		void DrawIndexed(uint32 index_count, uint32 instance_count = 1, uint32 index_offset = 0, uint32 base_vertex_location = 0, uint32 start_instance_location = 0);
		void Dispatch(uint32 group_count_x, uint32 group_count_y, uint32 group_count_z = 1);
		void DrawIndirect(GfxBuffer const& buffer, uint32 offset);
		void DrawIndexedIndirect(GfxBuffer const& buffer, uint32 offset);
		void DispatchIndirect(GfxBuffer const& buffer, uint32 offset);
		
		void CopyBuffer(GfxBuffer& dst, GfxBuffer const& src);
		void CopyBuffer(GfxBuffer& dst, uint64 dst_offset, GfxBuffer const& src, uint64 src_offset, uint64 size);
		void CopyTexture(GfxTexture& dst, GfxTexture const& src);
		void CopyTexture(GfxTexture& dst, uint32 dst_mip, uint32 dst_array, GfxTexture const& src, uint32 src_mip, uint32 src_array);

		void CopyStructureCount(GfxBuffer* dst_buffer, uint32 dst_buffer_offset, GfxReadWriteDescriptor src_view);

		GfxMappedSubresource MapBuffer(GfxBuffer* buffer, GfxMapType map_type);
		void UnmapBuffer(GfxBuffer* buffer);
		GfxMappedSubresource MapTexture(GfxTexture* texture, GfxMapType map_type, uint32	subresource = 0);
		void UnmapTexture(GfxTexture* texture, uint32 subresource = 0);

		void BeginRenderPass(GfxRenderPassDesc const& render_pass_desc);
		void EndRenderPass();

		void SetTopology(GfxPrimitiveTopology topology);
		void SetIndexBuffer(GfxBuffer* index_buffer, uint32 offset = 0);
		void SetVertexBuffers(std::span<GfxBuffer*> vertex_buffers, uint32 start_slot = 0);
		void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height);
		void SetScissorRect(uint32 x, uint32 y, uint32 width, uint32 height);

		void ClearReadWriteDescriptorFloat(GfxReadWriteDescriptor descriptor, const float v[4]);
		void ClearReadWriteDescriptorUint(GfxReadWriteDescriptor descriptor, const uint32 v[4]);
		void ClearRenderTarget(GfxColorDescriptor rtv, float const* clear_color);
		void ClearDepth(GfxDepthDescriptor dsv, float depth = 1.0f, uint8 stencil = 0, bool clear_stencil = false);
		void SetRenderTargets(std::span<GfxColorDescriptor> rtvs, GfxDepthDescriptor dsv = nullptr);

		void SetDepthStencilState(GfxDepthStencilState* dss, uint32 stencil_ref);
		void SetRasterizerState(GfxRasterizerState* rs);
		void SetBlendStateState(GfxBlendState* bs, float blend_factors[4], uint32  mask = 0xffffffff);

		void SetConstantBuffers(GfxShaderStage stage, uint32 start, std::span<GfxBuffer*> buffers);
		void SetVertexShader(GfxVertexShader* shader);
		void SetPixelShader(GfxPixelShader* shader);
		void SetHullShader(GfxHullShader* shader);
		void SetDomainShader(GfxDomainShader* shader);
		void SetGeometryShader(GfxGeometryShader* shader);
		void SetComputeShader(GfxComputeShader* shader);

		void SetReadOnlyDescriptors(GfxShaderStage stage, uint32 start, std::span<GfxReadOnlyDescriptor> descriptors);
		void SetReadWriteDescriptors(uint32 start, std::span<GfxReadWriteDescriptor> descriptors);

		void GenerateMips(GfxReadOnlyDescriptor srv);

		void Begin(GfxQuery& query);
		void End(GfxQuery& query);
		void GetData(GfxQuery& query, void* data, uint32 data_size);

		void BeginEvent(char const* event_name);
		void EndEvent();

	private:
		GfxDevice* gfx = nullptr;
		ArcPtr<ID3D11DeviceContext2> command_context = nullptr;
		ArcPtr<ID3DUserDefinedAnnotation> annot = nullptr;

		GfxVertexShader* current_vs = nullptr;
		GfxPixelShader* current_ps = nullptr;
		GfxHullShader* current_hs = nullptr;
		GfxDomainShader* current_ds = nullptr;
		GfxGeometryShader* current_gs = nullptr;
		GfxComputeShader* current_cs = nullptr;

		GfxBlendState current_blend_state{};
		GfxRasterizerState current_rasterizer_state{};
		GfxDepthStencilState current_depth_state{};
		GfxPrimitiveTopology current_topology = GfxPrimitiveTopology::Undefined;

		GfxRenderPassDesc* current_render_pass = nullptr;

		GfxInputLayout* current_il = nullptr;

		uint32 stencil_ref = 0;
		float prev_blendfactor[4] = {};
	};
}