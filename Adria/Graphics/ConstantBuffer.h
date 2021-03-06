#pragma once
#include <vector>
#include <d3d11.h>
#include <wrl.h>
#include "../Core/Definitions.h" 
#include "../Core/Macros.h" 
#include "ShaderCompiler.h"

namespace adria
{

	template<typename CBuffer>
	class ConstantBuffer
	{
		static constexpr uint32 GetCBufferSize(uint32 buffer_size)
		{
			return (buffer_size + (64 - 1)) & ~(64 - 1);
		}

	public:

		ConstantBuffer(ID3D11Device* device, bool dynamic = true);

		ConstantBuffer(ID3D11Device* device, CBuffer const& initialdata, bool dynamic = true);

		void Update(ID3D11DeviceContext* context, void const* data, uint32 data_size);

		void Update(ID3D11DeviceContext* context, CBuffer const& buffer_data);

		void Bind(ID3D11DeviceContext* context, EShaderStage stage, uint32 slot) const;

		ID3D11Buffer* const Buffer() const 
		{
			return buffer.Get();
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> buffer = nullptr;
		bool dynamic;
	};


	template<typename CBuffer>
	ConstantBuffer<CBuffer>::ConstantBuffer(ID3D11Device* device, CBuffer const& initialdata, bool dynamic /*= true*/) : dynamic{ dynamic }
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
		HRESULT hr = device->CreateBuffer(&bd, &sd, buffer.GetAddressOf());
		BREAK_IF_FAILED(hr);
	}

	template<typename CBuffer>
	ConstantBuffer<CBuffer>::ConstantBuffer(ID3D11Device* device, bool dynamic /*= true*/) : dynamic{ dynamic }
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bd.ByteWidth = GetCBufferSize(sizeof(CBuffer));
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

		HRESULT hr = device->CreateBuffer(&bd, nullptr, buffer.GetAddressOf());
		BREAK_IF_FAILED(hr);
	}

	template<typename CBuffer>
	void ConstantBuffer<CBuffer>::Update(ID3D11DeviceContext* context, void const* data, uint32 data_size)
	{
		D3D11_BUFFER_DESC desc{};
		buffer->GetDesc(&desc);

		if (desc.Usage == D3D11_USAGE_DYNAMIC)
		{
			D3D11_MAPPED_SUBRESOURCE mapped_buffer = {};
			ZeroMemory(&mapped_buffer, sizeof(D3D11_MAPPED_SUBRESOURCE));
			HRESULT hr = context->Map(buffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mapped_buffer);
			BREAK_IF_FAILED(hr);
			memcpy(mapped_buffer.pData, data, data_size);
			context->Unmap(buffer.Get(), 0);
		}
		else context->UpdateSubresource(buffer.Get(), 0, nullptr, &data, 0, 0);
	}

	template<typename CBuffer>
	void ConstantBuffer<CBuffer>::Update(ID3D11DeviceContext* context, CBuffer const& buffer_data)
	{
		Update(context, &buffer_data, sizeof(CBuffer));
	}

	template<typename CBuffer>
	void ConstantBuffer<CBuffer>::Bind(ID3D11DeviceContext* context, EShaderStage stage, uint32 slot) const
	{
		switch (stage)
		{
		case EShaderStage::VS:
			context->VSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
			break;
		case EShaderStage::PS:
			context->PSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
			break;
		case EShaderStage::HS:
			context->HSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
			break;
		case EShaderStage::DS:
			context->DSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
			break;
		case EShaderStage::GS:
			context->GSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
			break;
		case EShaderStage::CS:
			context->CSSetConstantBuffers(slot, 1, buffer.GetAddressOf());
			break;
		default:
			break;
		}
	}

}