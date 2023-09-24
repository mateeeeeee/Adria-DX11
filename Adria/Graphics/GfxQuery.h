#pragma once
#include "GfxDevice.h"

namespace adria
{
	enum class QueryType
	{
		Event = 0,
		Occlusion,
		Timestamp,
		TimstampDisjoint,
		PipelineStatistics,
		OcclusionPredicate
	};

	class GfxQuery
	{
		static constexpr D3D11_QUERY ConvertToD3D11Query(QueryType type)
		{
			switch (type)
			{
			case QueryType::Event:
				return D3D11_QUERY_EVENT;
			case QueryType::Occlusion:
				return D3D11_QUERY_OCCLUSION;
			case QueryType::Timestamp:
				return D3D11_QUERY_TIMESTAMP;
			case QueryType::TimstampDisjoint:
				return D3D11_QUERY_TIMESTAMP_DISJOINT;
			case QueryType::PipelineStatistics:
				return D3D11_QUERY_PIPELINE_STATISTICS;
			case QueryType::OcclusionPredicate:
			default:
				return D3D11_QUERY_OCCLUSION_PREDICATE;
			}
			return D3D11_QUERY_EVENT;
		}

	public:
		GfxQuery(GfxDevice* gfx, QueryType type)
		{
			D3D11_QUERY_DESC query_desc{};
			query_desc.Query = ConvertToD3D11Query(type);
			gfx->Device()->CreateQuery(&query_desc, query.GetAddressOf());
		}

		operator ID3D11Query*() const 
		{
			return query.Get();
		}

	private:
		ArcPtr<ID3D11Query> query;
	};
}