#include "GfxProfiler.h"
#include "GfxQuery.h"
#include "GfxDevice.h"
#include "GfxCommandContext.h"
#include "Core/Defines.h"
#include "Logging/Logger.h"

namespace adria
{
	void GfxProfiler::Initialize(GfxDevice* _gfx)
	{
		gfx = _gfx;
		for (auto& query : queries)
		{
			for (auto& block : query)
			{
				block.disjoint_query = std::make_unique<GfxQuery>(gfx, QueryType::TimestampDisjoint);
				block.timestamp_query_start = std::make_unique<GfxQuery>(gfx, QueryType::Timestamp);
				block.timestamp_query_end = std::make_unique<GfxQuery>(gfx, QueryType::Timestamp);
			}
		}
	}
	void GfxProfiler::Destroy()
	{
		name_to_index_map.clear();
		gfx = nullptr;
	}
	void GfxProfiler::NewFrame()
	{
		++current_frame;
		name_to_index_map.clear();
		scope_counter = 0;

		uint64 i = current_frame % FRAME_COUNT;
		for (auto& block : queries[i])
		{
			block.begin_called = false;
			block.end_called = false;
		}

	}


	void GfxProfiler::BeginProfileScope(GfxCommandContext* context, char const* name)
	{
		uint64 i = current_frame % FRAME_COUNT;
		uint32 profile_index = scope_counter++;
		name_to_index_map[name] = profile_index;

		QueryData& query_data = queries[i][profile_index];
		ADRIA_ASSERT(!query_data.begin_called);
		ADRIA_ASSERT(!query_data.end_called);
		context->BeginQuery(query_data.disjoint_query.get());
		context->EndQuery(query_data.timestamp_query_start.get());
		query_data.begin_called = true;
	}
	void GfxProfiler::EndProfileScope(GfxCommandContext* context, char const* name)
	{
		uint64 i = current_frame % FRAME_COUNT;
		uint32 profile_index = -1;
		profile_index = name_to_index_map[name];
		ADRIA_ASSERT(profile_index != uint32(-1));

		QueryData& query_data = queries[i][profile_index];
		ADRIA_ASSERT(query_data.begin_called);
		ADRIA_ASSERT(!query_data.end_called);
		context->EndQuery(query_data.timestamp_query_end.get());
		context->EndQuery(query_data.disjoint_query.get());
		query_data.end_called = true;
	}
	std::vector<Timestamp> GfxProfiler::GetProfilingResults()
	{
		GfxCommandContext* context = gfx->GetCommandContext();
		if (current_frame < FRAME_COUNT - 1)
		{
			++current_frame;
			return {};
		}

		uint64 old_index = (current_frame - FRAME_COUNT + 1) % FRAME_COUNT;
		auto& old_queries = queries[old_index];

		bool hr = true;
		QueryDataTimestampDisjoint disjoint_ts{};

		std::vector<Timestamp> results{};
		results.reserve(name_to_index_map.size());
		for (auto const& [name, index] : name_to_index_map)
		{
			ADRIA_ASSERT(index < MAX_QUERIES);
			QueryData& query = old_queries[index];
			if (query.begin_called && query.end_called)
			{
				while (!context->GetQueryData(query.disjoint_query.get(), nullptr, 0))
				{
					ADRIA_LOG(INFO, "Waiting for disjoint timestamp of %s in frame %llu", name.c_str(), current_frame);
					std::this_thread::sleep_for(std::chrono::nanoseconds(500));
				}
				hr = context->GetQueryData(query.disjoint_query.get(), &disjoint_ts, sizeof(QueryDataTimestampDisjoint));
				if (disjoint_ts.disjoint)
				{
					ADRIA_LOG(WARNING, "Disjoint Timestamp Flag in %s!", name.c_str());
				}
				else
				{
					uint64 begin_ts = 0;
					uint64 end_ts = 0;
					hr = context->GetQueryData(query.timestamp_query_start.get(), &begin_ts, sizeof(uint64));
					while (!context->GetQueryData(query.timestamp_query_end.get(), nullptr, 0))
					{
						ADRIA_LOG(INFO, "Waiting for disjoint timestamp of %s in frame %llu", name.c_str(), current_frame);
						std::this_thread::sleep_for(std::chrono::nanoseconds(500));
					}
					hr = context->GetQueryData(query.timestamp_query_end.get(), &end_ts, sizeof(uint64));

					float time_ms = (end_ts - begin_ts) * 1000.0f / disjoint_ts.frequency;
					std::string time_ms_string = std::to_string(time_ms);
					std::string result = name + " time: " + time_ms_string + "ms";

					results.push_back(Timestamp{ .name = name, .time_in_ms = time_ms });
				}
			}
			query.begin_called = false;
			query.end_called = false;
		}
		return results;
	}

	GfxProfiler::GfxProfiler() {}

	GfxProfiler::~GfxProfiler() {}

}