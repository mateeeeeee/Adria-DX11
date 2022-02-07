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

		template<typename index_t>
		void Create(ID3D11Device* device, index_t const* indices, uint32 index_count)
		{

			D3D11_BUFFER_DESC ibd{};

			ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			ibd.ByteWidth = static_cast<uint32>(index_count * sizeof(index_t));
			ibd.StructureByteStride = sizeof(index_t);

			ibd.CPUAccessFlags = 0;
			ibd.MiscFlags = 0;
			ibd.Usage = D3D11_USAGE_IMMUTABLE;

			D3D11_SUBRESOURCE_DATA data{};
			data.pSysMem = indices;
			device->CreateBuffer(&ibd, &data, buffer.GetAddressOf());
			this->index_count = static_cast<uint32>(index_count);
			if (std::is_same_v<index_t, uint32>) format = DXGI_FORMAT_R32_UINT;
			else if (std::is_same_v<index_t, uint16>) format = DXGI_FORMAT_R16_UINT;
			else format = DXGI_FORMAT_UNKNOWN;

		}

		template<typename index_t>
		void Create(ID3D11Device* device, std::vector<index_t> const& indices)
		{
			Create(device, indices.data(), (uint32)indices.size());
		}

		void Bind(ID3D11DeviceContext* context, uint32 offset = 0u) const
		{
			context->IASetIndexBuffer(buffer.Get(), format, offset);
		}

		uint32 Count() const
		{
			return index_count;
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
		DXGI_FORMAT format;
		uint32 index_count;

	};




}