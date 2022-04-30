#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <unordered_map>
#include <string>
#include <array>
#include "ProfilerSettings.h"

namespace adria
{

	class GPUProfiler
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

		GPUProfiler(ID3D11Device* device);

		void BeginProfileBlock(ID3D11DeviceContext* context, EProfilerBlock block);
		
		void EndProfileBlock(ID3D11DeviceContext* context, EProfilerBlock block);

		[[maybe_unused]] std::vector<std::string> GetProfilingResults(ID3D11DeviceContext* context, bool log_results = false);

	private:
		ID3D11Device* device;
		UINT64 current_frame = 0;
		std::array<std::array<QueryData, (size_t)EProfilerBlock::Count>, FRAME_COUNT> queries;
	};

	struct ScopedGPUProfileBlock
	{
		ScopedGPUProfileBlock(GPUProfiler& profiler, ID3D11DeviceContext* context, EProfilerBlock block)
			: profiler{ profiler }, block{ block }, context{ context }
		{
			profiler.BeginProfileBlock(context, block);
		}

		~ScopedGPUProfileBlock()
		{
			profiler.EndProfileBlock(context, block);
		}

		GPUProfiler& profiler;
		ID3D11DeviceContext* context;
		EProfilerBlock block;
	};

	#define SCOPED_GPU_PROFILE_BLOCK(profiler, context, block_id) ScopedGPUProfileBlock block(profiler, context, block_id)
	#define SCOPED_GPU_PROFILE_BLOCK_ON_CONDITION(profiler, context, block_id, cond) std::unique_ptr<ScopedGPUProfileBlock> scoped_profile = nullptr; \
																					  if(cond) scoped_profile = std::make_unique<ScopedGPUProfileBlock>(profiler, context, block_id)
}