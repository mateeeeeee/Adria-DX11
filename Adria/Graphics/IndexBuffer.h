#pragma once
#include <vector>
#include <d3d11.h>
#include <wrl.h>
#include "../Core/Definitions.h" 
#include "../Core/Macros.h" 

namespace adria
{
	class IndexBuffer
	{
	public:
		IndexBuffer() = default;

		void Create(ID3D11Device* device, void const* p_data, u32 index_count, u32 stride)
		{

			D3D11_BUFFER_DESC ibd{};

			ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			ibd.ByteWidth = index_count * stride;
			ibd.StructureByteStride = stride;
			ibd.CPUAccessFlags = 0;
			ibd.MiscFlags = 0;
			ibd.Usage = D3D11_USAGE_DEFAULT;


			D3D11_SUBRESOURCE_DATA data{};
			data.pSysMem = p_data;

			device->CreateBuffer(&ibd, &data, buffer.GetAddressOf());

			this->index_count = index_count;

			format = DXGI_FORMAT_R32_UINT;

		}

		template<typename index_t>
		void Create(ID3D11Device* device, index_t const* indices, u32 index_count)
		{

			D3D11_BUFFER_DESC ibd{};

			ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			ibd.ByteWidth = static_cast<u32>(index_count * sizeof(index_t));
			ibd.StructureByteStride = sizeof(index_t);

			ibd.CPUAccessFlags = 0;
			ibd.MiscFlags = 0;
			ibd.Usage = D3D11_USAGE_DEFAULT;

			D3D11_SUBRESOURCE_DATA data{};
			data.pSysMem = indices;
			device->CreateBuffer(&ibd, &data, buffer.GetAddressOf());
			this->index_count = static_cast<u32>(index_count);
			if (std::is_same_v<index_t, u32>) format = DXGI_FORMAT_R32_UINT;
			else if (std::is_same_v<index_t, u16>) format = DXGI_FORMAT_R16_UINT;
			else format = DXGI_FORMAT_UNKNOWN;

		}

		template<typename index_t>
		void Create(ID3D11Device* device, std::vector<index_t> const& indices)
		{
			Create(device, indices.data(), (u32)indices.size());
		}

		void Bind(ID3D11DeviceContext* context, u32 offset = 0u) const
		{
			context->IASetIndexBuffer(buffer.Get(), format, offset);
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
		DXGI_FORMAT format;
		u32 index_count;

	};




}