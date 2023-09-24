#pragma once
#include <string>
#include <array>
#include <unordered_map>
#include <d3d11.h>
#include "GfxDefines.h"
#include "Utilities/Singleton.h"

namespace adria
{
	struct Timestamp
	{
		std::string name;
		float time_in_ms;
	};

	class GfxProfiler : public Singleton<GfxProfiler>
	{
		friend class Singleton<GfxProfiler>;

		static constexpr uint64 FRAME_COUNT = GFX_BACKBUFFER_COUNT;
		static constexpr uint64 MAX_QUERIES = 256;
		struct QueryData
		{
			ArcPtr<ID3D11Query> disjoint_query;
			ArcPtr<ID3D11Query> timestamp_query_start;
			ArcPtr<ID3D11Query> timestamp_query_end;
			bool begin_called, end_called;
		};

	public:
		void Initialize(ID3D11Device* device);
		void Destroy();
		void NewFrame();
		void BeginProfileScope(ID3D11DeviceContext* context, char const* name);
		void EndProfileScope(ID3D11DeviceContext* context, char const* name);
		[[maybe_unused]] std::vector<Timestamp> GetProfilingResults(ID3D11DeviceContext* context);

	private:
		ID3D11Device* device = nullptr;
		uint64 current_frame = 0;
		std::array<std::array<QueryData, MAX_QUERIES>, FRAME_COUNT> queries;
		std::unordered_map<std::string, uint32> name_to_index_map;
		uint32 scope_counter = 0;

	private:
		GfxProfiler() {}
		~GfxProfiler() {}
	};
	#define g_GfxProfiler GfxProfiler::Get()

#if GFX_PROFILING
	struct GfxProfileScope
	{
		GfxProfileScope(ID3D11DeviceContext* context, char const* name, bool active = true)
			: name{ name }, context{ context }, active{ active }
		{
			if(active) 
				g_GfxProfiler.BeginProfileScope(context, name);
		}

		~GfxProfileScope()
		{
			if (active) 
				g_GfxProfiler.EndProfileScope(context, name);
		}

		ID3D11DeviceContext* context;
		char const* name;
		bool active;
	};

	#define AdriaGfxProfileScope(context, name) GfxProfileScope ADRIA_CONCAT(gfx_profile, __COUNTER__)(context, name)
	#define AdriaGfxProfileCondScope(context, name, cond) GfxProfileScope ADRIA_CONCAT(gfx_profile, __COUNTER__)(context, name, cond)
#else
	#define AdriaGfxProfileScope(context, name) 
	#define AdriaGfxProfileCondScope(context, name, active) 
#endif
}