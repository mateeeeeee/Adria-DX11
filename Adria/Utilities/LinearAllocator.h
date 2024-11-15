#pragma once
#include "AllocatorUtil.h"

namespace adria
{

	class LinearAllocator
	{
	public:

		LinearAllocator(OffsetType max_size, OffsetType reserve = 0) noexcept :
			max_size{ max_size }, reserve{ reserve }, top{ reserve }
		{}
		LinearAllocator(LinearAllocator const&) = default;
		LinearAllocator& operator = (LinearAllocator const&) = delete;
		LinearAllocator(LinearAllocator&&) = default;
		LinearAllocator& operator=(LinearAllocator&&) = delete;
		~LinearAllocator() = default;

		OffsetType Allocate(OffsetType size, OffsetType align = 0)
		{
			auto aligned_top = Align(top, align);

			if (aligned_top + size >= max_size) return INVALID_OFFSET;
			else
			{
				OffsetType start = aligned_top;

				aligned_top += size;

				top = aligned_top;

				return start;
			}
		}

		void Clear()
		{
			top = reserve;
		}

		OffsetType MaxSize()  const { return max_size; }
		Bool Full()			  const { return top == max_size; };
		Bool Empty()		  const { return top == reserve; };
		OffsetType UsedSize() const { return top; }

	private:
		OffsetType const max_size;
		OffsetType const reserve;
		OffsetType top;
	};

}
