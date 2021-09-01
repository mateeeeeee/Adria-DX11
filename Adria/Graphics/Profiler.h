#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <unordered_map>
#include <string>
#include <array>
#include "ProfilerFlags.h"

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

		struct FrameQueries
		{
			std::unordered_map<ProfilerFlags, QueryData> block_queries;
			Microsoft::WRL::ComPtr<ID3D11Query> frame_disjoint_query;
			Microsoft::WRL::ComPtr<ID3D11Query> frame_timestamp_query_start;
			Microsoft::WRL::ComPtr<ID3D11Query> frame_timestamp_query_end;
			bool begin_called, end_called;
		};

	public:
		Profiler(ID3D11Device* device);

		void AddBlockProfiling(ProfilerFlags flag);

		void BeginFrameProfiling(ID3D11DeviceContext* context);

		void EndFrameProfiling(ID3D11DeviceContext* context);

		void BeginBlockProfiling(ID3D11DeviceContext* context, ProfilerFlags flag);
		
		void EndBlockProfiling(ID3D11DeviceContext* context, ProfilerFlags flag);

		std::vector<std::string> GetProfilingResults(ID3D11DeviceContext* context, bool log_results = false);

	private:
		ID3D11Device* device;
		UINT64 current_frame = 0;
		std::array<FrameQueries, FRAME_COUNT> queries;
	};

	struct ScopedProfileBlock
	{
		ScopedProfileBlock(Profiler& profiler, ID3D11DeviceContext* context, ProfilerFlags flag)
			: profiler{ profiler }, flag{ flag }, context{ context }
		{
			profiler.BeginBlockProfiling(context, flag);
		}

		~ScopedProfileBlock()
		{
			profiler.EndBlockProfiling(context, flag);
		}

		Profiler& profiler;
		ID3D11DeviceContext* context;
		ProfilerFlags flag;
	};

	#define DECLARE_SCOPED_PROFILE_BLOCK(profiler, context, flag) ScopedProfileBlock block(profiler, context, flag)
	#define DECLARE_SCOPED_PROFILE_BLOCK_ON_CONDITION(profiler, context, flag, flags) std::unique_ptr<ScopedProfileBlock> block = nullptr; \
																					  if(flags & flag) block = std::make_unique<ScopedProfileBlock>(profiler, context, flag)
}