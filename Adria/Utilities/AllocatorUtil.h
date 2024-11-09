#pragma once

namespace adria
{

	inline Uint64 Align(Uint64 address, Uint64 align)
	{
		if ((0 == align) || (align & (align - 1))) return address;

		return ((address + (align - 1)) & ~(align - 1));
	}

	using OffsetType = Uint64;

	inline constexpr OffsetType const INVALID_OFFSET = static_cast<OffsetType>(-1);

}