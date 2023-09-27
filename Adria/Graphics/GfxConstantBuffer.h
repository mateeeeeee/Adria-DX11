#pragma once
#include "GfxBuffer.h"
#include "GfxDevice.h"
#include "GfxCommandContext.h"

namespace adria
{
	template<typename CBuffer>
	class GfxConstantBuffer
	{
		static constexpr uint32 GetCBufferSize(uint32 buffer_size)
		{
			return (buffer_size + (64 - 1)) & ~(64 - 1);
		}

	public:

		GfxConstantBuffer(GfxDevice* gfx, bool dynamic = true);
		GfxConstantBuffer(GfxDevice* gfx, CBuffer const& initialdata, bool dynamic = true);

		void Update(GfxCommandContext* context, void const* data, uint32 data_size);
		void Update(GfxCommandContext* context, CBuffer const& buffer_data);

		void Bind(GfxCommandContext* context, GfxShaderStage stage, uint32 slot) const;
		GfxBuffer* Buffer() const
		{
			return buffer.get();
		}

	private:
		std::unique_ptr<GfxBuffer> buffer = nullptr;
		bool dynamic;
	};


	template<typename CBuffer>
	GfxConstantBuffer<CBuffer>::GfxConstantBuffer(GfxDevice* gfx, CBuffer const& initialdata, bool dynamic /*= true*/) : dynamic{ dynamic }
	{
		GfxBufferDesc desc{};
		desc.resource_usage = dynamic ? GfxResourceUsage::Dynamic : GfxResourceUsage::Default;
		desc.size = GetCBufferSize(sizeof(CBuffer));
		desc.bind_flags = GfxBindFlag::ConstantBuffer;
		desc.cpu_access = dynamic ? GfxCpuAccess::Write : GfxCpuAccess::None;

		GfxBufferInitialData data = initialdata;
		buffer = std::make_unique<GfxBuffer>(gfx, desc, data);
	}

	template<typename CBuffer>
	GfxConstantBuffer<CBuffer>::GfxConstantBuffer(GfxDevice* gfx, bool dynamic /*= true*/) : dynamic{ dynamic }
	{
		GfxBufferDesc desc{};
		desc.resource_usage = dynamic ? GfxResourceUsage::Dynamic : GfxResourceUsage::Default;
		desc.size = GetCBufferSize(sizeof(CBuffer));
		desc.bind_flags = GfxBindFlag::ConstantBuffer;
		desc.cpu_access = dynamic ? GfxCpuAccess::Write : GfxCpuAccess::None;

		buffer = std::make_unique<GfxBuffer>(gfx, desc);
	}

	template<typename CBuffer>
	void GfxConstantBuffer<CBuffer>::Update(GfxCommandContext* context, void const* data, uint32 data_size)
	{
		context->UpdateBuffer(buffer.get(), data, data_size);
	}

	template<typename CBuffer>
	void GfxConstantBuffer<CBuffer>::Update(GfxCommandContext* context, CBuffer const& buffer_data)
	{
		Update(context, &buffer_data, sizeof(CBuffer));
	}

	template<typename CBuffer>
	void GfxConstantBuffer<CBuffer>::Bind(GfxCommandContext* context, GfxShaderStage stage, uint32 slot) const
	{
		context->SetConstantBuffer(stage, slot, buffer.get());
	}

}