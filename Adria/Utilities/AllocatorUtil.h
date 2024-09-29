#pragma once

namespace adria
{

	inline size_t Align(size_t address, size_t align)
	{
		if ((0 == align) || (align & (align - 1))) return address;

		return ((address + (align - 1)) & ~(align - 1));
	}

	using OffsetType = size_t;

	inline constexpr OffsetType const INVALID_OFFSET = static_cast<OffsetType>(-1);

}