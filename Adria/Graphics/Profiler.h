#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <unordered_map>
#include <string>
#include <array>
#include "ProfilerSettings.h"

namespace adria
{

	class Profiler
	{
		static constexpr UINT64 FRAME_COUNT = 3;

		struct QueryData
		{
			Microsoft::WRL::ComPtr<ID3D11Query> disjoint_query;
			Microsoft::WRL::ComPtr<ID3D11Query> timestamp_query_start;
			Microsoft::WRL::ComPtr<ID3D11Query> timestamp_query_end;
			bool begin_called, end_called;
		};

	public:

		Profiler(ID3D11Device* device);

		void BeginBlockProfiling(ID3D11DeviceContext* context, ProfilerBlock block);
		
		void EndBlockProfiling(ID3D11DeviceContext* context, ProfilerBlock block);

		std::vector<std::string> GetProfilingResults(ID3D11DeviceContext* context, bool log_results = false);

	private:
		ID3D11Device* device;
		UINT64 current_frame = 0;
		std::array<std::array<QueryData, (size_t)ProfilerBlock::eCount>, FRAME_COUNT> queries;
	};

	struct ScopedProfileBlock
	{
		ScopedProfileBlock(Profiler& profiler, ID3D11DeviceContext* context, ProfilerBlock block)
			: profiler{ profiler }, block{ block }, context{ context }
		{
			profiler.BeginBlockProfiling(context, block);
		}

		~ScopedProfileBlock()
		{
			profiler.EndBlockProfiling(context, block);
		}

		Profiler& profiler;
		ID3D11DeviceContext* context;
		ProfilerBlock block;
	};

	#define DECLARE_SCOPED_PROFILE_BLOCK(profiler, context, block_id) ScopedProfileBlock block(profiler, context, block_id)
	#define DECLARE_SCOPED_PROFILE_BLOCK_ON_CONDITION(profiler, context, block_id, cond) std::unique_ptr<ScopedProfileBlock> scoped_profile = nullptr; \
																					  if(cond) scoped_profile = std::make_unique<ScopedProfileBlock>(profiler, context, block_id)
}