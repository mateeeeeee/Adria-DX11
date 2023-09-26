#pragma once
#include "Utilities/EnumUtil.h"

namespace adria
{
	enum GfxSubresourceType : uint8
	{
		GfxSubresourceType_SRV,
		GfxSubresourceType_UAV,
		GfxSubresourceType_RTV,
		GfxSubresourceType_DSV,
		GfxSubresourceType_Invalid
	};

	enum class GfxBindFlag : uint32
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
	DEFINE_ENUM_BIT_OPERATORS(GfxBindFlag);

	inline constexpr uint32 ParseBindFlags(GfxBindFlag flags)
	{
		uint32 result = 0;
		if (HasAnyFlag(flags, GfxBindFlag::VertexBuffer))
			result |= D3D11_BIND_VERTEX_BUFFER;
		if (HasAnyFlag(flags, GfxBindFlag::IndexBuffer))
			result |= D3D11_BIND_INDEX_BUFFER;
		if (HasAnyFlag(flags, GfxBindFlag::ConstantBuffer))
			result |= D3D11_BIND_CONSTANT_BUFFER;
		if (HasAnyFlag(flags, GfxBindFlag::ShaderResource))
			result |= D3D11_BIND_SHADER_RESOURCE;
		if (HasAnyFlag(flags, GfxBindFlag::RenderTarget))
			result |= D3D11_BIND_RENDER_TARGET;
		if (HasAnyFlag(flags, GfxBindFlag::DepthStencil))
			result |= D3D11_BIND_DEPTH_STENCIL;
		if (HasAnyFlag(flags, GfxBindFlag::UnorderedAccess))
			result |= D3D11_BIND_UNORDERED_ACCESS;
		return result;
	}

	enum class GfxResourceUsage : uint8
	{
		Default,
		Immutable,
		Dynamic,
		Staging,
	};
	inline constexpr D3D11_USAGE ConvertUsage(GfxResourceUsage value)
	{
		switch (value)
		{
		case GfxResourceUsage::Default:
			return D3D11_USAGE_DEFAULT;
			break;
		case GfxResourceUsage::Immutable:
			return D3D11_USAGE_IMMUTABLE;
			break;
		case GfxResourceUsage::Dynamic:
			return D3D11_USAGE_DYNAMIC;
			break;
		case GfxResourceUsage::Staging:
			return D3D11_USAGE_STAGING;
			break;
		default:
			break;
		}
		return D3D11_USAGE_DEFAULT;
	}

	enum class GfxCpuAccess : uint8
	{
		None = 0b00,
		Write = 0b01,
		Read = 0b10,
		ReadWrite = 0b11
	};
	DEFINE_ENUM_BIT_OPERATORS(GfxCpuAccess);

	inline constexpr uint32 ParseCPUAccessFlags(GfxCpuAccess value)
	{
		uint32 result = 0;
		if (HasAnyFlag(value, GfxCpuAccess::Write))
			result |= D3D11_CPU_ACCESS_WRITE;
		if (HasAnyFlag(value, GfxCpuAccess::Read))
			result |= D3D11_CPU_ACCESS_READ;
		return result;
	}

	enum class GfxTextureMiscFlag : uint32
	{
		None = 0,
		TextureCube = 1 << 0,
		GenerateMips = 1 << 1
	};
	DEFINE_ENUM_BIT_OPERATORS(GfxTextureMiscFlag);

	inline constexpr uint32 ParseMiscFlags(GfxTextureMiscFlag value)
	{
		uint32 result = 0;
		if (HasAnyFlag(value, GfxTextureMiscFlag::TextureCube))
			result |= D3D11_RESOURCE_MISC_TEXTURECUBE;
		if (HasAnyFlag(value, GfxTextureMiscFlag::GenerateMips))
			result |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
		return result;
	}

	enum class GfxBufferMiscFlag : uint32
	{
		None,
		IndirectArgs = 1 << 0,
		BufferRaw = 1 << 1,
		BufferStructured = 1 << 2
	};
	DEFINE_ENUM_BIT_OPERATORS(GfxBufferMiscFlag);

	inline constexpr uint32 ParseMiscFlags(GfxBufferMiscFlag value)
	{
		uint32 result = 0;
		if (HasAnyFlag(value, GfxBufferMiscFlag::IndirectArgs))
			result |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
		if (HasAnyFlag(value, GfxBufferMiscFlag::BufferRaw))
			result |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		if (HasAnyFlag(value, GfxBufferMiscFlag::BufferStructured))
			result |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		return result;
	}

	enum class GfxMapType : uint32 
	{
		Read = 1,
		Write = 2,
		ReadWrite = 3,
		WriteDiscard = 4,
		WriteNoOverwrite = 5
	};
	inline constexpr D3D11_MAP ConvertMapType(GfxMapType value)
	{
		switch (value)
		{
		case GfxMapType::Read:
			return D3D11_MAP_READ;
		case GfxMapType::Write:
			return D3D11_MAP_WRITE;
		case GfxMapType::ReadWrite:
			return D3D11_MAP_READ_WRITE;
		case GfxMapType::WriteDiscard:
			return D3D11_MAP_WRITE_DISCARD;
		case GfxMapType::WriteNoOverwrite:
			return D3D11_MAP_WRITE_NO_OVERWRITE;
		default:
			return D3D11_MAP_WRITE;
		}
		return D3D11_MAP_WRITE;
	}

	struct GfxMappedSubresource
	{
		void* p_data;
		uint32 row_pitch;
		uint32 depth_pitch;
	};
}