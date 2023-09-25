#pragma once
#include <d3d11.h>

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

	class GfxDevice;
	class GfxQuery
	{
	public:
		GfxQuery(GfxDevice* gfx, QueryType type);

		operator ID3D11Query*() const 
		{
			return query.Get();
		}

	private:
		ArcPtr<ID3D11Query> query;
	};
}