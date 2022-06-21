#pragma once
#include <memory>
#include <optional>
#include "GraphicsDeviceDX11.h"
#include "../Utilities/EnumUtil.h"
#include "../Utilities/StringUtil.h"
#include "../Core/Macros.h"


namespace adria
{
	inline constexpr uint32_t GetFormatStride(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_SNORM:
		case DXGI_FORMAT_BC4_UNORM:
			return 8u;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 16u;
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 12u;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return 8u;
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			return 8u;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return 4u;
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
			return 2u;
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
			return 1u;
		default:
			ADRIA_ASSERT(false);
			break;
		}

		return 16u;
	}

	enum ESubresourceType : uint8
	{
		SubresourceType_SRV,
		SubresourceType_UAV,
		SubresourceType_RTV,
		SubresourceType_DSV,
		SubresourceType_Invalid
	};

	enum class EBindFlag : uint32
	{
		None = 0,
		ShaderResource = 1 << 0,
		RenderTarget = 1 << 1,
		DepthStencil = 1 << 2,
		UnorderedAccess = 1 << 3,
		VertexBuffer = 1 << 4,
		IndexBuffer = 1 << 5,
		ConstantBuffer = 1 << 6
	};
	DEFINE_ENUM_BIT_OPERATORS(EBindFlag);

	inline constexpr uint32 ParseBindFlags(EBindFlag flags)
	{
		uint32 result = 0;
		if (HasAnyFlag(flags, EBindFlag::VertexBuffer))
			result |= D3D11_BIND_VERTEX_BUFFER;
		if (HasAnyFlag(flags, EBindFlag::IndexBuffer))
			result |= D3D11_BIND_INDEX_BUFFER;
		if (HasAnyFlag(flags, EBindFlag::ConstantBuffer))
			result |= D3D11_BIND_CONSTANT_BUFFER;
		if (HasAnyFlag(flags, EBindFlag::ShaderResource))
			result |= D3D11_BIND_SHADER_RESOURCE;
		if (HasAnyFlag(flags, EBindFlag::RenderTarget))
			result |= D3D11_BIND_RENDER_TARGET;
		if (HasAnyFlag(flags, EBindFlag::DepthStencil))
			result |= D3D11_BIND_DEPTH_STENCIL;
		if (HasAnyFlag(flags, EBindFlag::UnorderedAccess))
			result |= D3D11_BIND_UNORDERED_ACCESS;
		return result;
	}

	enum class EResourceUsage : uint8
	{
		Default,
		Immutable,
		Dynamic,
		Staging,
	};
	inline constexpr D3D11_USAGE ConvertUsage(EResourceUsage value)
	{
		switch (value)
		{
		case EResourceUsage::Default:
			return D3D11_USAGE_DEFAULT;
			break;
		case EResourceUsage::Immutable:
			return D3D11_USAGE_IMMUTABLE;
			break;
		case EResourceUsage::Dynamic:
			return D3D11_USAGE_DYNAMIC;
			break;
		case EResourceUsage::Staging:
			return D3D11_USAGE_STAGING;
			break;
		default:
			break;
		}
		return D3D11_USAGE_DEFAULT;
	}

	enum class ECpuAccess : uint8
	{
		None = 0b00,
		Write = 0b01,
		Read = 0b10,
		ReadWrite = 0b11
	};
	DEFINE_ENUM_BIT_OPERATORS(ECpuAccess);

	inline constexpr uint32 ParseCPUAccessFlags(ECpuAccess value)
	{
		uint32_t result = 0;
		if (HasAnyFlag(value, ECpuAccess::Write))
			result |= D3D11_CPU_ACCESS_WRITE;
		if (HasAnyFlag(value, ECpuAccess::Read))
			result |= D3D11_CPU_ACCESS_READ;
		return result;
	}

	enum class ETextureMiscFlag : uint32
	{
		None = 0,
		TextureCube = 1 << 0,
		GenerateMips = 1 << 1
	};
	DEFINE_ENUM_BIT_OPERATORS(ETextureMiscFlag);

	inline constexpr uint32 ParseMiscFlags(ETextureMiscFlag value)
	{
		uint32 result = 0;
		if (HasAnyFlag(value, ETextureMiscFlag::TextureCube))
			result |= D3D11_RESOURCE_MISC_TEXTURECUBE;
		return result;
	}

	enum class EBufferMiscFlag : uint32
	{
		None,
		IndirectArgs = 1 << 0,
		BufferRaw = 1 << 1,
		BufferStructured = 1 << 2
	};
	DEFINE_ENUM_BIT_OPERATORS(EBufferMiscFlag);

	inline constexpr uint32 ParseMiscFlags(EBufferMiscFlag value)
	{
		uint32 result = 0;
		if (HasAnyFlag(value, EBufferMiscFlag::IndirectArgs))
			result |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
		if (HasAnyFlag(value, EBufferMiscFlag::BufferRaw))
			result |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		if (HasAnyFlag(value, EBufferMiscFlag::BufferStructured))
			result |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		return result;
	}
}