#pragma once
#include "GfxBuffer.h"
#include "GfxDevice.h"
#include "GfxCommandContext.h"

namespace adria
{
	template<typename CBuffer>
	class GfxConstantBuffer
	{
		static constexpr Uint32 GetCBufferSize()
		{
			return (sizeof(CBuffer) + (64 - 1)) & ~(64 - 1);
		}

	public:

		GfxConstantBuffer(GfxDevice* gfx, Bool dynamic = true);
		GfxConstantBuffer(GfxDevice* gfx, CBuffer const& initialdata, Bool dynamic = true);

		void Update(GfxCommandContext* context, void const* data, Uint32 data_size);
		void Update(GfxCommandContext* context, CBuffer const& buffer_data);

		void Bind(GfxCommandContext* context, GfxShaderStage stage, Uint32 slot) const;
		GfxBuffer* Buffer() const
		{
			return buffer.get();
		}

	private:
		std::unique_ptr<GfxBuffer> buffer = nullptr;
		Bool dynamic;
	};


	template<typename CBuffer>
	GfxConstantBuffer<CBuffer>::GfxConstantBuffer(GfxDevice* gfx, CBuffer const& initialdata, Bool dynamic) : dynamic{ dynamic }
	{
		GfxBufferDesc desc{};
		desc.resource_usage = dynamic ? GfxResourceUsage::Dynamic : GfxResourceUsage::Default;
		desc.size = GetCBufferSize();
		desc.bind_flags = GfxBindFlag::ConstantBuffer;
		desc.cpu_access = dynamic ? GfxCpuAccess::Write : GfxCpuAccess::None;

		GfxBufferInitialData data = initialdata;
		buffer = std::make_unique<GfxBuffer>(gfx, desc, data);
	}

	template<typename CBuffer>
	GfxConstantBuffer<CBuffer>::GfxConstantBuffer(GfxDevice* gfx, Bool dynamic) : dynamic{ dynamic }
	{
		GfxBufferDesc desc{};
		desc.resource_usage = dynamic ? GfxResourceUsage::Dynamic : GfxResourceUsage::Default;
		desc.size = GetCBufferSize();
		desc.bind_flags = GfxBindFlag::ConstantBuffer;
		desc.cpu_access = dynamic ? GfxCpuAccess::Write : GfxCpuAccess::None;

		buffer = std::make_unique<GfxBuffer>(gfx, desc);
	}

	template<typename CBuffer>
	void GfxConstantBuffer<CBuffer>::Update(GfxCommandContext* context, void const* data, Uint32 data_size)
	{
		context->UpdateBuffer(buffer.get(), data, data_size);
	}

	template<typename CBuffer>
	void GfxConstantBuffer<CBuffer>::Update(GfxCommandContext* context, CBuffer const& buffer_data)
	{
		Update(context, &buffer_data, sizeof(CBuffer));
	}

	template<typename CBuffer>
	void GfxConstantBuffer<CBuffer>::Bind(GfxCommandContext* context, GfxShaderStage stage, Uint32 slot) const
	{
		context->SetConstantBuffer(stage, slot, buffer.get());
	}

}