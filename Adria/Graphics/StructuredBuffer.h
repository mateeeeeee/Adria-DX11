#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <vector>
#include "../Core/Definitions.h" 
#include "../Core/Macros.h" 


namespace adria
{

    //refactor this

	template<typename T>
	class StructuredBuffer
	{
	public:
        
        StructuredBuffer(ID3D11Device* device, u32 element_count,
            u32 bind_flags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
            bool dynamic = false) : element_count{ element_count }
        {
            CD3D11_BUFFER_DESC desc(sizeof(T) * element_count, bind_flags,
                dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
                dynamic ? D3D11_CPU_ACCESS_WRITE : 0,
                D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
                sizeof(T));

            device->CreateBuffer(&desc, nullptr, &buffer);

            if (bind_flags & D3D11_BIND_UNORDERED_ACCESS) 
                device->CreateUnorderedAccessView(buffer.Get(), nullptr, &uav);

            if (bind_flags & D3D11_BIND_SHADER_RESOURCE) 
                device->CreateShaderResourceView(buffer.Get(), nullptr, &srv);
        }

        StructuredBuffer(StructuredBuffer const&) = delete;
        StructuredBuffer& operator=(StructuredBuffer const&) = delete;

        ~StructuredBuffer() = default;

        ID3D11Buffer* Buffer() { return buffer.Get(); }
        auto UAV() const { return uav.Get(); }
        auto SRV() const { return srv.Get(); }

        void Update(ID3D11DeviceContext* context, void const* data, u64 data_size)
        {
            if (data_size == 0) return;

            D3D11_BUFFER_DESC buffer_desc{};
            buffer->GetDesc(&buffer_desc);
            ADRIA_ASSERT(buffer_desc.Usage == D3D11_USAGE_DYNAMIC && "Cannot Update Non-dynamic Structured Buffer");
            
            D3D11_MAPPED_SUBRESOURCE mapped_buffer = {};
            HRESULT hr = context->Map(buffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mapped_buffer);
            BREAK_IF_FAILED(hr);
            memcpy(mapped_buffer.pData, data, data_size);
            context->Unmap(buffer.Get(), 0u);
        }

        void Update(ID3D11DeviceContext* context, std::vector<T> const& structure_data)
        {
            if (structure_data.empty()) return;
            ADRIA_ASSERT(structure_data.size() <= element_count);
            Update(context, structure_data.data(), sizeof(T) * structure_data.size());
        }

        T* Map(ID3D11DeviceContext* context)
        {
            D3D11_MAPPED_SUBRESOURCE mapped_buffer = {};
            HRESULT hr = context->Map(buffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mapped_buffer);
            BREAK_IF_FAILED(hr);
            return static_cast<T*>(mapped_buffer.pData);
        }

        void Unmap(ID3D11DeviceContext* context)
        {
            context->Unmap(buffer.Get(), 0u);
        }

    private:

        u32 const element_count;
        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
	};

}