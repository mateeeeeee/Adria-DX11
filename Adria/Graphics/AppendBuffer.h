#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <vector>
#include "../Core/Definitions.h" 
#include "../Core/Macros.h" 


namespace adria
{

	template<typename T>
	class AppendBuffer
	{
	public:

		AppendBuffer(ID3D11Device* device, U32 element_count) : element_count{ element_count }
		{
			CD3D11_BUFFER_DESC desc(sizeof(T) * element_count, D3D11_BIND_UNORDERED_ACCESS,
				D3D11_USAGE_DEFAULT,
				0,
				D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
				sizeof(T));

			device->CreateBuffer(&desc, nullptr, &buffer);

			D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
			uav_desc.Format = DXGI_FORMAT_UNKNOWN;
			uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.FirstElement = 0;
			uav_desc.Buffer.NumElements = element_count;
			uav_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
			
			device->CreateUnorderedAccessView(buffer.Get(), &uav_desc, &uav);
		}

		AppendBuffer(AppendBuffer const&) = delete;
		AppendBuffer& operator=(AppendBuffer const&) = delete;
		~AppendBuffer() = default;

		ID3D11Buffer* Buffer() { return buffer.Get(); }
		auto UAV() const { return uav.Get(); }
		
	private:

		U32 const element_count;
		Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
	};

}