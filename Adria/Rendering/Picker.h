#pragma once
#include <memory>
#include "ShaderManager.h"
#include "Graphics/GfxBuffer.h"
#include "Graphics/GfxDevice.h" 
#include "Graphics/GfxCommandContext.h" 
#include "Graphics/GfxShaderProgram.h" 

namespace adria
{

	struct PickingData
	{
		Vector4 position = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		Vector4 normal = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
	};

	class Picker
	{
		friend class Renderer;
		
	private:

		Picker(GfxDevice* gfx) : gfx(gfx), picking_buffer(nullptr)
		{
			GfxBufferDesc desc{};
			desc.size = sizeof(PickingData);
			desc.stride = sizeof(PickingData);
			desc.cpu_access = GfxCpuAccess::Read;
			desc.misc_flags = GfxBufferMiscFlag::BufferStructured;
			desc.resource_usage = GfxResourceUsage::Default;
			desc.bind_flags = GfxBindFlag::UnorderedAccess;
			picking_buffer = std::make_unique<GfxBuffer>(gfx, desc);
			picking_buffer->CreateUAV();
		}

		PickingData Pick(GfxReadOnlyDescriptor depth_srv, GfxReadOnlyDescriptor normal_srv)
		{
			GfxCommandContext* command_context = gfx->GetCommandContext();
			ID3D11DeviceContext* context = command_context->GetNative();
			ID3D11ShaderResourceView* shader_views[2] = { depth_srv, normal_srv };
			context->CSSetShaderResources(0, ARRAYSIZE(shader_views), shader_views);
			ID3D11UnorderedAccessView* lights_uav = picking_buffer->UAV();
			context->CSSetUnorderedAccessViews(0, 1, &lights_uav, nullptr);

			ShaderManager::GetShaderProgram(ShaderProgram::Picker)->Bind(command_context);
			context->Dispatch(1, 1, 1);

			ID3D11ShaderResourceView* null_srv[2] = { nullptr };
			context->CSSetShaderResources(0, ARRAYSIZE(null_srv), null_srv);
			ID3D11UnorderedAccessView* null_uav = nullptr;
			context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);

			PickingData const* data = (PickingData const*)picking_buffer->MapForRead();
			PickingData picking_data = *data;
			picking_buffer->Unmap();
			return picking_data;
		}

	private:

		GfxDevice* gfx;
		std::unique_ptr<GfxBuffer> picking_buffer;
	};
}