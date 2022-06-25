#include "GPUProfiler.h"
#include "../Core/Macros.h"
#include "../Logging/Logger.h"



namespace adria
{
	GPUProfiler::GPUProfiler(ID3D11Device* device) : device(device), queries{}
	{
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

	void GPUProfiler::BeginProfileBlock(ID3D11DeviceContext* context, EProfilerBlock block)
	{
		UINT64 i = current_frame % FRAME_COUNT;
		QueryData& query_data = queries[i][static_cast<size_t>(block)];
		ADRIA_ASSERT(!query_data.begin_called);
		ADRIA_ASSERT(!query_data.end_called);
		context->Begin(query_data.disjoint_query.Get());
		context->End(query_data.timestamp_query_start.Get());
		query_data.begin_called = true;
	}

	void GPUProfiler::EndProfileBlock(ID3D11DeviceContext* context, EProfilerBlock block)
	{
		UINT64 i = current_frame % FRAME_COUNT;
		QueryData& query_data = queries[i][static_cast<size_t>(block)];
		ADRIA_ASSERT(query_data.begin_called);
		ADRIA_ASSERT(!query_data.end_called);
		context->End(query_data.timestamp_query_end.Get());
		context->End(query_data.disjoint_query.Get());
		query_data.end_called = true;
	}

	std::vector<std::string> GPUProfiler::GetProfilingResults(ID3D11DeviceContext* context, bool log_results)
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
		for (size_t i = 0; i < old_queries.size(); ++i)
		{
			QueryData& query = old_queries[i];
			EProfilerBlock block = static_cast<EProfilerBlock>(i);
			if (query.begin_called && query.end_called)
			{
				while (context->GetData(query.disjoint_query.Get(), NULL, 0, 0) == S_FALSE)
				{
					ADRIA_LOG(INFO, "Waiting for disjoint timestamp of %s in frame %llu", ToString(block).c_str(), current_frame);
					std::this_thread::sleep_for(std::chrono::nanoseconds(500));
				}

				hr = context->GetData(query.disjoint_query.Get(), &disjoint_ts, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);
				if (disjoint_ts.Disjoint)
				{
					ADRIA_LOG(WARNING, "Disjoint Timestamp Flag in %s!", ToString(block).c_str());
				}
				else
				{
					UINT64 begin_ts = 0;
					UINT64 end_ts = 0;

					hr = context->GetData(query.timestamp_query_start.Get(), &begin_ts, sizeof(UINT64), 0);

					while (context->GetData(query.timestamp_query_end.Get(), NULL, 0, 0) == S_FALSE)
					{
						ADRIA_LOG(INFO, "Waiting for frame end timestamp of %s in frame %llu", ToString(block).c_str(), current_frame);
						std::this_thread::sleep_for(std::chrono::nanoseconds(500));
					}
					hr = context->GetData(query.timestamp_query_end.Get(), &end_ts, sizeof(UINT64), 0);

					FLOAT time_ms = (end_ts - begin_ts) * 1000.0f / disjoint_ts.Frequency;

					std::string time_ms_string = std::to_string(time_ms);
					std::string result = ToString(block) + " time: " + time_ms_string + "ms";

					results.push_back(result);
					if (log_results)
					{
						ADRIA_LOG(INFO, result.c_str());
					}
				}
			}
			query.begin_called = false;
			query.end_called = false;
		}

		++current_frame;
		return results;
	}

}