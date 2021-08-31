#include "Profiler.h"

#include "../Logging/Logger.h"



namespace adria
{
	Profiler::Profiler(ID3D11Device* device) : device(device), queries{}
	{

		D3D11_QUERY_DESC query_desc{};

		for (auto& query : queries)
		{
			query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			device->CreateQuery(&query_desc, query.frame_disjoint_query.GetAddressOf());
			query_desc.Query = D3D11_QUERY_TIMESTAMP;
			device->CreateQuery(&query_desc, query.frame_timestamp_query_start.GetAddressOf());
			device->CreateQuery(&query_desc, query.frame_timestamp_query_end.GetAddressOf());
		}

	}

	void Profiler::AddBlockProfiling(ProfilerFlags block)
	{
		D3D11_QUERY_DESC query_desc{};

		for (auto& query : queries)
		{
			query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
			device->CreateQuery(&query_desc, query.block_queries[block].disjoint_query.GetAddressOf());
			query_desc.Query = D3D11_QUERY_TIMESTAMP;
			device->CreateQuery(&query_desc, query.block_queries[block].timestamp_query_start.GetAddressOf());
			device->CreateQuery(&query_desc, query.block_queries[block].timestamp_query_end.GetAddressOf());
		}
	}

	void Profiler::BeginFrameProfiling(ID3D11DeviceContext* context)
	{
		UINT64 i = current_frame % FRAME_COUNT;

		context->Begin(queries[i].frame_disjoint_query.Get());
		context->End(queries[i].frame_timestamp_query_start.Get());

		queries[i].begin_called = true;
	}

	void Profiler::EndFrameProfiling(ID3D11DeviceContext* context)
	{
		UINT64 i = current_frame % FRAME_COUNT;

		context->End(queries[i].frame_timestamp_query_end.Get());
		context->End(queries[i].frame_disjoint_query.Get());

		queries[i].end_called = true;
	}

	void Profiler::BeginBlockProfiling(ID3D11DeviceContext* context, ProfilerFlags flag)
	{
		UINT64 i = current_frame % FRAME_COUNT;

		if (auto it = queries[i].block_queries.find(flag); it == queries[i].block_queries.end())
			Log::Error("Timestamp Query with name " + ToString(flag) + " does not exist\n");
		else
		{
			context->Begin(it->second.disjoint_query.Get());
			context->End(it->second.timestamp_query_start.Get());
			it->second.begin_called = true;
		}

	}

	void Profiler::EndBlockProfiling(ID3D11DeviceContext* context, ProfilerFlags flag)
	{
		UINT64 i = current_frame % FRAME_COUNT;

		if (auto it = queries[i].block_queries.find(flag); it == queries[i].block_queries.end())
			Log::Error("Timestamp Query with name " + ToString(flag) + " does not exist\n");
		else
		{
			context->End(it->second.timestamp_query_end.Get());
			context->End(it->second.disjoint_query.Get());
			it->second.end_called = true;
		}
	}

	std::vector<std::string> Profiler::GetProfilingResults(ID3D11DeviceContext* context, bool log_results)
	{
		if (current_frame < FRAME_COUNT - 1)
		{
			++current_frame;
			return {};
		}

		UINT64 old_index = (current_frame - FRAME_COUNT + 1) % FRAME_COUNT;
		auto& old_queries = queries[old_index];


		HRESULT hr = S_OK;
		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_ts{};

		std::vector<std::string> results{};
		if (old_queries.begin_called && old_queries.end_called)
		{
			while (context->GetData(old_queries.frame_disjoint_query.Get(), NULL, 0, 0) == S_FALSE)
			{
				Log::Info("Waiting for frame disjoint timestamp in frame " + std::to_string(current_frame) + "\n");
				std::this_thread::sleep_for(std::chrono::nanoseconds(500));
			}

			hr = context->GetData(old_queries.frame_disjoint_query.Get(), &disjoint_ts, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);
			if (disjoint_ts.Disjoint)
			{
				Log::Warning("Disjoint Timestamp Flag!\n");
			}
			else
			{
				UINT64 begin_frame_ts = 0;
				UINT64 end_frame_ts = 0;

				hr = context->GetData(old_queries.frame_timestamp_query_start.Get(), &begin_frame_ts, sizeof(UINT64), 0);

				while (context->GetData(old_queries.frame_timestamp_query_end.Get(), NULL, 0, 0) == S_FALSE)
				{
					Log::Info("Waiting for frame end timestamp in frame " + std::to_string(current_frame) + "\n");
					std::this_thread::sleep_for(std::chrono::nanoseconds(500));
				}
				hr = context->GetData(old_queries.frame_timestamp_query_end.Get(), &end_frame_ts, sizeof(UINT64), 0);

				FLOAT time_ms = (end_frame_ts - begin_frame_ts) * 1000.0f / disjoint_ts.Frequency;

				std::string time_ms_string = std::to_string(time_ms);
				std::string result = "Frame time: " + time_ms_string + "ms\n";
				results.push_back(result);
				if(log_results) Log::Info(result);
			}

		}

		old_queries.begin_called = false;
		old_queries.end_called = false;

		
		for (auto& [flag, query] : old_queries.block_queries)
		{
			if (query.begin_called && query.end_called)
			{
				while (context->GetData(query.disjoint_query.Get(), NULL, 0, 0) == S_FALSE)
				{
					Log::Info("Waiting for disjoint timestamp of " + ToString(flag) + " in frame " + std::to_string(current_frame) + "\n");
					std::this_thread::sleep_for(std::chrono::nanoseconds(500));
				}

				hr = context->GetData(query.disjoint_query.Get(), &disjoint_ts, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);
				if (disjoint_ts.Disjoint)
				{
					Log::Warning("Disjoint Timestamp Flag in " + ToString(flag) + "!\n");
				}
				else
				{
					UINT64 begin_ts = 0;
					UINT64 end_ts = 0;

					hr = context->GetData(query.timestamp_query_start.Get(), &begin_ts, sizeof(UINT64), 0);

					while (context->GetData(query.timestamp_query_end.Get(), NULL, 0, 0) == S_FALSE)
					{
						Log::Info("Waiting for frame end timestamp in frame " + std::to_string(current_frame) + "\n");
						std::this_thread::sleep_for(std::chrono::nanoseconds(500));
					}
					hr = context->GetData(query.timestamp_query_end.Get(), &end_ts, sizeof(UINT64), 0);

					FLOAT time_ms = (end_ts - begin_ts) * 1000.0f / disjoint_ts.Frequency;

					std::string time_ms_string = std::to_string(time_ms);
					std::string result = ToString(flag) + " time: " + time_ms_string + "ms\n";

					results.push_back(result);
					if(log_results) Log::Info(result);
				}
			}
			query.begin_called = false;
			query.end_called = false;
		}
		++current_frame;

		return results;
	}

}