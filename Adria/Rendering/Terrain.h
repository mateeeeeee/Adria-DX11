#pragma once
#include <vector>
#include <DirectXMath.h>
#include "Graphics/GfxVertexFormat.h"

namespace adria
{
	class Terrain
	{
	public:
		Terrain(std::vector<TexturedNormalVertex> const& terrain_vertices, Float tx, Float tz, Uint64 xcount, Uint64 zcount);

		Float HeightAt(Float x, Float z) const;
		Vector3 NormalAt(Float x, Float z) const;
		std::pair<Float, Float> TileSizes() const 
		{
			return { tile_size_x, tile_size_z };
		}
		std::pair<Uint64, Uint64> TileCounts() const
		{
			return { tile_count_x, tile_count_z };
		}

	private:
		std::vector<TexturedNormalVertex> terrain_vertices;
		Float tile_size_x;
		Float tile_size_z;
		Uint64 tile_count_x;
		Uint64 tile_count_z;
		Vector3 offset;

	private:

		Uint64 GetIndex(Uint64 x_, Uint64 z_) const
		{
			return x_ + z_ * (tile_count_x + 1);
		}

		Bool CheckIndex(Uint64 i) const
		{
			return i >= 0 && i < terrain_vertices.size();
		}
	};
}