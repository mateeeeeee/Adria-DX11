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
		OcclusionPredicate,
	};

	class GfxQuery
	{
		static D3D11_QUERY ConvertQueryType(QueryType type)
		{
			return D3D11_QUERY{};
		}
	public:
		GfxQuery(GfxDevice* gfx, QueryType type)
		{
			D3D11_QUERY_DESC query_desc{};
			query_desc.Query = ConvertQueryType(type);
			gfx->Device()->CreateQuery(&query_desc, query.GetAddressOf());
		}

		operator ID3D11Query* () const 
		{
			return query.Get();
		}

	private:
		ArcPtr<ID3D11Query> query;
	};
}