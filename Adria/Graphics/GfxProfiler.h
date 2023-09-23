#pragma once
#include <d3d11.h>
#include <string>
#include <array>
#include "GfxProfilerSettings.h"

namespace adria
{
	struct Timestamp
	{
		std::string name;
		float time_in_ms;
	};

	class GfxProfiler
	{
		static constexpr UINT64 FRAME_COUNT = GFX_BACKBUFFER_COUNT;
		struct QueryData
		{
			Microsoft::WRL::ComPtr<ID3D11Query> disjoint_query;
			Microsoft::WRL::ComPtr<ID3D11Query> timestamp_query_start;
			Microsoft::WRL::ComPtr<ID3D11Query> timestamp_query_end;
			bool begin_called, end_called;
		};

	public:
		explicit GfxProfiler(ID3D11Device* device);

		void BeginProfileBlock(ID3D11DeviceContext* context, EProfilerBlock block);
		void EndProfileBlock(ID3D11DeviceContext* context, EProfilerBlock block);
		[[maybe_unused]] std::vector<Timestamp> GetProfilingResults(ID3D11DeviceContext* context);

	private:
		ID3D11Device* device;
		UINT64 current_frame = 0;
		std::array<std::array<QueryData, (size_t)EProfilerBlock::Count>, FRAME_COUNT> queries;
	};

	struct GfxScopedProfileBlock
	{
		GfxScopedProfileBlock(GfxProfiler& profiler, ID3D11DeviceContext* context, EProfilerBlock block)
			: profiler{ profiler }, block{ block }, context{ context }
		{
			profiler.BeginProfileBlock(context, block);
		}

		~GfxScopedProfileBlock()
		{
			profiler.EndProfileBlock(context, block);
		}

		GfxProfiler& profiler;
		ID3D11DeviceContext* context;
		EProfilerBlock block;
	};

	#define SCOPED_GPU_PROFILE_BLOCK(profiler, context, block_id) GfxScopedProfileBlock block(profiler, context, block_id)
	#define SCOPED_GPU_PROFILE_BLOCK_ON_CONDITION(profiler, context, block_id, cond) std::unique_ptr<GfxScopedProfileBlock> scoped_profile = nullptr; \
																					  if(cond) scoped_profile = std::make_unique<GfxScopedProfileBlock>(profiler, context, block_id)
}