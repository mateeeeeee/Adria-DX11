#pragma once
#include <vector>
#include "ResourceCommon.h"

namespace adria
{
	struct BufferDesc
	{
		uint64 size = 0;
		EResourceUsage resource_usage = EResourceUsage::Default;
		ECpuAccess cpu_access = ECpuAccess::None;
		EBindFlag bind_flags = EBindFlag::None;
		EBufferMiscFlag misc_flags = EBufferMiscFlag::None;
		uint32 stride = 0; //structured buffers, (vertex buffers, index buffers, needed for count calculation not for srv as structured buffers)
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN; //typed buffers, index buffers
		std::strong_ordering operator<=>(BufferDesc const& other) const = default;
	};

	static BufferDesc VertexBufferDesc(uint64 vertex_count, uint32 stride)
	{
		BufferDesc desc{};
		desc.bind_flags = EBindFlag::None;
		desc.resource_usage = EResourceUsage::Default;
		desc.size = vertex_count * stride;
		desc.stride = stride;
		desc.misc_flags = EBufferMiscFlag::None;
		return desc;
	}
	static BufferDesc IndexBufferDesc(uint64 index_count, bool small_indices)
	{
		BufferDesc desc{};
		desc.bind_flags = EBindFlag::None;
		desc.resource_usage = EResourceUsage::Default;
		desc.stride = small_indices ? 2 : 4;
		desc.size = index_count * desc.stride;
		desc.misc_flags = EBufferMiscFlag::None;
		return desc;
	}

	template<typename T>
	static BufferDesc StructuredBufferDesc(uint64 count, bool uav = true, bool dynamic = false)
	{
		BufferDesc desc{};
		desc.resource_usage = (uav || !dynamic) ? EResourceUsage::Default : EResourceUsage::Dynamic;
		desc.bind_flags = EBindFlag::ShaderResource;
		if (uav) desc.bind_flags |= EBindFlag::UnorderedAccess;
		desc.misc_flags = EBufferMiscFlag::BufferStructured;
		desc.stride = sizeof(T);
		desc.size = desc.stride * count;
		return desc;
	}
	static BufferDesc CounterBufferDesc()
	{
		BufferDesc desc{};
		desc.size = sizeof(uint32);
		desc.bind_flags = EBindFlag::UnorderedAccess;
		return desc;
	}

	struct BufferSubresourceDesc
	{
		uint64 offset = 0;
		uint64 size = uint64(-1);
		std::strong_ordering operator<=>(BufferSubresourceDesc const& other) const = default;
	};

	using BufferInitialData = void const*;

	class Buffer
	{
	public:
		Buffer(GraphicsDevice* gfx, BufferDesc const& desc, BufferInitialData initial_data = nullptr)
			: gfx(gfx), desc(desc)
		{
			D3D11_BUFFER_DESC buffer_desc{};
			buffer_desc.ByteWidth = desc.size;
			buffer_desc.Usage = ConvertUsage(desc.resource_usage);
			buffer_desc.BindFlags = ParseBindFlags(desc.bind_flags);
			buffer_desc.CPUAccessFlags = ParseCPUAccessFlags(desc.cpu_access);
			buffer_desc.MiscFlags = ParseMiscFlags(desc.misc_flags);
			buffer_desc.StructureByteStride = desc.stride;

			D3D11_SUBRESOURCE_DATA data{};
			if (initial_data != nullptr)
			{
				data.pSysMem = initial_data;
				data.SysMemPitch = desc.size;
				data.SysMemSlicePitch = 0;
			}
			ID3D11Device* device = gfx->Device();
			HRESULT hr = device->CreateBuffer(&buffer_desc, initial_data == nullptr ? nullptr : &data, resource.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}

		Buffer(Buffer const&) = delete;
		Buffer& operator=(Buffer const&) = delete;
		~Buffer() = default;

		ID3D11ShaderResourceView* GetSubresource_SRV(size_t i = 0) const { return srvs[i].Get(); }
		ID3D11UnorderedAccessView* GetSubresource_UAV(size_t i = 0) const { return uavs[i].Get(); }

		[[maybe_unused]] size_t CreateSubresource_SRV(BufferSubresourceDesc const* desc = nullptr)
		{
			BufferSubresourceDesc _desc = desc ? *desc : BufferSubresourceDesc{};
			return CreateSubresource(SubresourceType_SRV, _desc);
		}
		[[maybe_unused]] size_t CreateSubresource_UAV(BufferSubresourceDesc const* desc = nullptr)
		{
			BufferSubresourceDesc _desc = desc ? *desc : BufferSubresourceDesc{};
			return CreateSubresource(SubresourceType_UAV, _desc);
		}

		ID3D11Buffer* GetNative() const { return resource.Get(); }
		ID3D11Buffer* Detach()
		{
			return resource.Detach();
		}

		BufferDesc const& GetDesc() const { return desc; }
		UINT GetCount() const
		{
			ADRIA_ASSERT(desc.stride != 0);
			return static_cast<UINT>(desc.size / desc.stride);
		}

		[[maybe_unused]] void* Map()
		{
			ID3D11DeviceContext* ctx = gfx->Context();
			if (desc.resource_usage == EResourceUsage::Dynamic)
			{
				D3D11_MAPPED_SUBRESOURCE mapped_buffer{};
				ZeroMemory(&mapped_buffer, sizeof(D3D11_MAPPED_SUBRESOURCE));
				HRESULT hr = ctx->Map(resource.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mapped_buffer);
				BREAK_IF_FAILED(hr);
				return mapped_buffer.pData;
			}
			ADRIA_ASSERT(false);
		}
		void Unmap()
		{
			ID3D11DeviceContext* ctx = gfx->Context();
			ctx->Unmap(resource.Get(), 0);
		}
		void Update(void const* src_data, size_t data_size)
		{
			ID3D11DeviceContext* ctx = gfx->Context();
			if (desc.resource_usage == EResourceUsage::Dynamic)
			{
				D3D11_MAPPED_SUBRESOURCE mapped_buffer{};
				ZeroMemory(&mapped_buffer, sizeof(D3D11_MAPPED_SUBRESOURCE));
				HRESULT hr = ctx->Map(resource.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mapped_buffer);
				BREAK_IF_FAILED(hr);
				memcpy(mapped_buffer.pData, src_data, data_size);
				ctx->Unmap(resource.Get(), 0);
			}
			else ctx->UpdateSubresource(resource.Get(), 0, nullptr, src_data, 0, 0);
		}
		template<typename T>
		void Update(T const& src_data)
		{
			Update(&src_data, sizeof(T));
		}

	private:
		GraphicsDevice* gfx;
		Microsoft::WRL::ComPtr<ID3D11Buffer> resource;
		BufferDesc desc;
		std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> srvs;
		std::vector<Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>> uavs;

	private:
		size_t CreateSubresource(ESubresourceType type, BufferSubresourceDesc const& subresource_desc)
		{
			HRESULT hr = E_FAIL;
			ID3D11Device* device = gfx->Device();
			switch (type)
			{
			case SubresourceType_SRV:
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
				if (HasAnyFlag(desc.misc_flags, EBufferMiscFlag::BufferRaw))
				{
					// This is a Raw Buffer
					srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
					srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
					srv_desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
					srv_desc.BufferEx.FirstElement = (UINT)subresource_desc.offset / sizeof(uint32_t);
					srv_desc.BufferEx.NumElements = std::min(subresource_desc.size, desc.size - subresource_desc.offset) / sizeof(uint32);
				}
				else if (HasAnyFlag(desc.misc_flags, EBufferMiscFlag::BufferStructured))
				{
					// This is a Structured Buffer
					srv_desc.Format = DXGI_FORMAT_UNKNOWN;
					srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
					srv_desc.BufferEx.FirstElement = (UINT)subresource_desc.offset / desc.stride;
					srv_desc.BufferEx.NumElements = std::min(subresource_desc.size, desc.size - subresource_desc.offset) / desc.stride;
				}
				else
				{
					// This is a Typed Buffer
					uint32_t stride = GetFormatStride(desc.format);
					srv_desc.Format = desc.format;
					srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
					srv_desc.Buffer.FirstElement = (UINT)subresource_desc.offset / stride;
					srv_desc.Buffer.NumElements = std::min(subresource_desc.size, desc.size - subresource_desc.offset) / stride;
				}

				Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
				hr = device->CreateShaderResourceView(resource.Get(), &srv_desc, &srv);

				if (SUCCEEDED(hr))
				{
					srvs.push_back(srv);
					return srvs.size() - 1;
				}
				else ADRIA_ASSERT(false && "Failed SRV Creation");
			}
			break;
			case SubresourceType_UAV:
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
				uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
				if (HasAnyFlag(desc.misc_flags, EBufferMiscFlag::BufferRaw))
				{
					// This is a Raw Buffer
					uav_desc.Format = DXGI_FORMAT_R32_TYPELESS; 
					uav_desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
					uav_desc.Buffer.FirstElement = (UINT)subresource_desc.offset / sizeof(uint32);
					uav_desc.Buffer.NumElements = std::min(subresource_desc.size, desc.size - subresource_desc.offset) / sizeof(uint32);
				}
				else if (HasAnyFlag(desc.misc_flags, EBufferMiscFlag::BufferStructured))
				{
					// This is a Structured Buffer
					uav_desc.Format = DXGI_FORMAT_UNKNOWN;      
					uav_desc.Buffer.FirstElement = (UINT)subresource_desc.offset / desc.stride;
					uav_desc.Buffer.NumElements = std::min(subresource_desc.size, desc.size - subresource_desc.offset) / desc.stride;
				}
				else
				{
					// This is a Typed Buffer
					uint32_t stride = GetFormatStride(desc.format);
					uav_desc.Format = desc.format;
					uav_desc.Buffer.FirstElement = (UINT)subresource_desc.offset / stride;
					uav_desc.Buffer.NumElements = std::min(subresource_desc.size, desc.size - subresource_desc.offset) / stride;
				}

				Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> uav;
				hr = device->CreateUnorderedAccessView(resource.Get(), &uav_desc, &uav);

				if (SUCCEEDED(hr))
				{
					uavs.push_back(uav);
					return uavs.size() - 1;
				}
				else ADRIA_ASSERT(false && "Failed UAV Creation");
			}
			break;
			default:
				break;
			}
			return -1;
		}
	};
}