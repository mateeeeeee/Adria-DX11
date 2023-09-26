#pragma once
#include <d3d11.h>

namespace adria
{
	enum class QueryType
	{
		Event = 0,
		Occlusion,
		Timestamp,
		TimestampDisjoint,
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

	struct QueryDataTimestampDisjoint
	{
		uint64 frequency;
		bool32 disjoint;
	};

	struct QueryDataPipelineStatistics
	{
		uint64 IAVertices;
		uint64 IAPrimitives;
		uint64 VSInvocations;
		uint64 GSInvocations;
		uint64 GSPrimitives;
		uint64 CInvocations;
		uint64 CPrimitives;
		uint64 PSInvocations;
		uint64 HSInvocations;
		uint64 DSInvocations;
		uint64 CSInvocations;
	};
}