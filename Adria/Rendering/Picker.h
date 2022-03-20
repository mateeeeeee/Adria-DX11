#pragma once
#include <memory>
#include <DirectXMath.h>
#include "../Graphics/StructuredBuffer.h"
#include "../Graphics/GraphicsCoreDX11.h" //
#include "../Graphics/ShaderProgram.h" 

#include "../Logging/Logger.h"

namespace adria
{

	struct PickingData
	{
		DirectX::XMFLOAT4 position = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		DirectX::XMFLOAT4 normal   = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	};

	class Picker
	{
		friend class Renderer;
		
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
			ID3D11ShaderResourceView* shader_views[2] = { depth_srv, normal_srv };
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

			ADRIA_LOG(INFO, "Pick position: %f %f %f", data->position.x, data->position.y, data->position.z);

			picking_buffer->Unmap(context);
			return picking_data;
		}

	private:

		GraphicsCoreDX11* gfx;
		std::unique_ptr<StructuredBuffer<PickingData>> picking_buffer;
		ComputeProgram pick_program;
	};
}