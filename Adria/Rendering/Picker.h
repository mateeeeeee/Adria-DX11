#pragma once
#include <memory>
#include <DirectXMath.h>
#include "../Graphics/StructuredBuffer.h"
#include "../Graphics/GraphicsCoreDX11.h" //
#include "../Graphics/ShaderProgram.h" 


namespace adria
{


	class Picker
	{
		friend class Renderer;

		struct PickingData
		{
			DirectX::XMFLOAT4 position;
			DirectX::XMFLOAT4 normal;
		};
		
	private:
		Picker(GraphicsCoreDX11* gfx) : gfx(gfx), picking_buffer(nullptr)
		{
			picking_buffer = std::make_unique<StructuredBuffer<PickingData>>(gfx->Device(), 1, false, false, true);

			ShaderBlob cs_blob;
			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/PickerCS.cso", cs_blob);
			pick_program.Create(gfx->Device(), cs_blob);
		}

		PickingData Pick(ID3D11ShaderResourceView* depth_srv, ID3D11ShaderResourceView* normal_srv)
		{
			ID3D11DeviceContext* context = gfx->Context();
			ID3D11ShaderResourceView* shader_views[2] = { nullptr };
			shader_views[0] = depth_srv;
			shader_views[1] = normal_srv;
			context->CSSetShaderResources(0, ARRAYSIZE(shader_views), shader_views);
			ID3D11UnorderedAccessView* lights_uav = picking_buffer->UAV();
			context->CSSetUnorderedAccessViews(0, 1, &lights_uav, nullptr);

			pick_program.Bind(context);
			context->Dispatch(1, 1, 1);

			ID3D11ShaderResourceView* null_srv[2] = { nullptr };
			context->CSSetShaderResources(0, ARRAYSIZE(null_srv), null_srv);
			ID3D11UnorderedAccessView* null_uav = nullptr;
			context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);

			PickingData const* data = picking_buffer->MapForRead(context);
			PickingData picking_data = *data;
			picking_buffer->Unmap(context);
			return picking_data;
		}

	private:
		GraphicsCoreDX11* gfx;
		std::unique_ptr<StructuredBuffer<PickingData>> picking_buffer;
		ComputeProgram pick_program;
	};
}