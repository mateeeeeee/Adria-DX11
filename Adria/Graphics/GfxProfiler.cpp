#include "GfxProfiler.h"
#include "Core/Defines.h"
#include "Logging/Logger.h"



namespace adria
{
	void GfxProfiler::Initialize(ID3D11Device* _device) 
	{
		device = _device;
		D3D11_QUERY_DESC query_desc{};
		for (auto& query : queries)
		{
			for (auto& block : query)
			{
				query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
				device->CreateQuery(&query_desc, block.disjoint_query.GetAddressOf());
				query_desc.Query = D3D11_QUERY_TIMESTAMP;
				device->CreateQuery(&query_desc, block.timestamp_query_start.GetAddressOf());
				device->CreateQuery(&query_desc, block.timestamp_query_end.GetAddressOf());
			}
		}
	}

	void GfxProfiler::Destroy()
	{
		name_to_index_map.clear();
		device = nullptr;
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

	void GfxProfiler::BeginProfileScope(ID3D11DeviceContext* context, char const* name)
	{
		uint64 i = current_frame % FRAME_COUNT;
		uint32 profile_index = scope_counter++;
		name_to_index_map[name] = profile_index;

		QueryData& query_data = queries[i][profile_index];
		ADRIA_ASSERT(!query_data.begin_called);
		ADRIA_ASSERT(!query_data.end_called);
		context->Begin(query_data.disjoint_query.Get());
		context->End(query_data.timestamp_query_start.Get());
		query_data.begin_called = true;
	}

	void GfxProfiler::EndProfileScope(ID3D11DeviceContext* context, char const* name)
	{
		uint64 i = current_frame % FRAME_COUNT;
		uint32 profile_index = -1;
		profile_index = name_to_index_map[name];
		ADRIA_ASSERT(profile_index != uint32(-1));

		QueryData& query_data = queries[i][profile_index];
		ADRIA_ASSERT(query_data.begin_called);
		ADRIA_ASSERT(!query_data.end_called);
		context->End(query_data.timestamp_query_end.Get());
		context->End(query_data.disjoint_query.Get());
		query_data.end_called = true;
	}

	std::vector<Timestamp> GfxProfiler::GetProfilingResults(ID3D11DeviceContext* context)
	{
		if (current_frame < FRAME_COUNT - 1)
		{
			++current_frame;
			return {};
		}

		uint64 old_index = (current_frame - FRAME_COUNT + 1) % FRAME_COUNT;
		auto& old_queries = queries[old_index];

		HRESULT hr = S_OK;
		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_ts{};

		std::vector<Timestamp> results{};
		results.reserve(name_to_index_map.size());
		for (auto const& [name, index] : name_to_index_map)
		{
			ADRIA_ASSERT(index < MAX_QUERIES);
			QueryData& query = old_queries[index];
			if (query.begin_called && query.end_called)
			{
				while (context->GetData(query.disjoint_query.Get(), NULL, 0, 0) == S_FALSE)
				{
					ADRIA_LOG(INFO, "Waiting for disjoint timestamp of %s in frame %llu", name.c_str(), current_frame);
					std::this_thread::sleep_for(std::chrono::nanoseconds(500));
				}
				hr = context->GetData(query.disjoint_query.Get(), &disjoint_ts, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);
				if (disjoint_ts.Disjoint)
				{
					ADRIA_LOG(WARNING, "Disjoint Timestamp Flag in %s!", name.c_str());
				}
				else
				{
					uint64 begin_ts = 0;
					uint64 end_ts = 0;
					hr = context->GetData(query.timestamp_query_start.Get(), &begin_ts, sizeof(uint64), 0);
					while (context->GetData(query.timestamp_query_end.Get(), NULL, 0, 0) == S_FALSE)
					{
						ADRIA_LOG(INFO, "Waiting for frame end timestamp of %s in frame %llu", name.c_str(), current_frame);
						std::this_thread::sleep_for(std::chrono::nanoseconds(500));
					}
					hr = context->GetData(query.timestamp_query_end.Get(), &end_ts, sizeof(uint64), 0);

					float time_ms = (end_ts - begin_ts) * 1000.0f / disjoint_ts.Frequency;
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

}