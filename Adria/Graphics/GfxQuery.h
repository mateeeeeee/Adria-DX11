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
		Ref<ID3D11Query> query;
	};

	struct QueryDataTimestampDisjoint
	{
		Uint64 frequency;
		Bool32 disjoint;
	};

	struct QueryDataPipelineStatistics
	{
		Uint64 IAVertices;
		Uint64 IAPrimitives;
		Uint64 VSInvocations;
		Uint64 GSInvocations;
		Uint64 GSPrimitives;
		Uint64 CInvocations;
		Uint64 CPrimitives;
		Uint64 PSInvocations;
		Uint64 HSInvocations;
		Uint64 DSInvocations;
		Uint64 CSInvocations;
	};
}