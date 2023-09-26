#pragma once
#include "GfxDevice.h"

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

		void Update(ID3D11DeviceContext* context, void const* data, uint32 data_size);
		void Update(ID3D11DeviceContext* context, CBuffer const& buffer_data);

		void Bind(ID3D11DeviceContext* context, GfxShaderStage stage, uint32 slot) const;
		ID3D11Buffer* const Buffer() const 
		{
			return buffer.Get();
		}

	private:
		ArcPtr<ID3D11Buffer> buffer = nullptr;
		bool dynamic;
	};


	template<typename CBuffer>
	GfxConstantBuffer<CBuffer>::GfxConstantBuffer(GfxDevice* gfx, CBuffer const& initialdata, bool dynamic /*= true*/) : dynamic{ dynamic }
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bd.ByteWidth = GetCBufferSize(sizeof(CBuffer));
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

		D3D11_SUBRESOURCE_DATA sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.pSysMem = (void*)&initialdata;
		HRESULT hr = gfx->GetDevice()->CreateBuffer(&bd, &sd, buffer.GetAddressOf());
		GFX_CHECK_HR(hr);
	}

	template<typename CBuffer>
	GfxConstantBuffer<CBuffer>::GfxConstantBuffer(GfxDevice* gfx, bool dynamic /*= true*/) : dynamic{ dynamic }
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bd.ByteWidth = GetCBufferSize(sizeof(CBuffer));
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

		HRESULT hr = gfx->GetDevice()->CreateBuffer(&bd, nullptr, buffer.GetAddressOf());
		GFX_CHECK_HR(hr);
	}

	template<typename CBuffer>
	void GfxConstantBuffer<CBuffer>::Update(ID3D11DeviceContext* context, void const* data, uint32 data_size)
	{
		D3D11_BUFFER_DESC desc{};
		buffer->GetDesc(&desc);

		if (desc.Usage == D3D11_USAGE_DYNAMIC)
		{
			D3D11_MAPPED_SUBRESOURCE mapped_buffer = {};
			ZeroMemory(&mapped_buffer, sizeof(D3D11_MAPPED_SUBRESOURCE));
			HRESULT hr = context->Map(buffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mapped_buffer);
			GFX_CHECK_HR(hr);
			memcpy(mapped_buffer.pData, data, data_size);
			context->Unmap(buffer.Get(), 0);
		}
		else context->UpdateSubresource(buffer.Get(), 0, nullptr, &data, data_size, 0);
	}

	template<typename CBuffer>
	void GfxConstantBuffer<CBuffer>::Update(ID3D11DeviceContext* context, CBuffer const& buffer_data)
	{
		Update(context, &buffer_data, sizeof(CBuffer));
	}

	template<typename CBuffer>
	void GfxConstantBuffer<CBuffer>::Bind(ID3D11DeviceContext* context, GfxShaderStage stage, uint32 slot) const
	{
		switch (stage)
		{
		case GfxShaderStage::VS:
			context->VSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
			break;
		case GfxShaderStage::PS:
			context->PSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
			break;
		case GfxShaderStage::HS:
			context->HSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
			break;
		case GfxShaderStage::DS:
			context->DSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
			break;
		case GfxShaderStage::GS:
			context->GSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
			break;
		case GfxShaderStage::CS:
			context->CSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
			break;
		default:
			break;
		}
	}

}