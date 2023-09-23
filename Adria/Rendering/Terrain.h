#pragma once
#include <vector>
#include <DirectXMath.h>
#include "Core/CoreTypes.h"
#include "Graphics/GfxVertexTypes.h"

namespace adria
{
	class Terrain
	{
	public:
		Terrain(std::vector<TexturedNormalVertex> const& terrain_vertices, float tx, float tz, uint64 xcount, uint64 zcount);

		float HeightAt(float x, float z) const;
		
		DirectX::XMFLOAT3 NormalAt(float x, float z) const;

		std::pair<float, float> TileSizes() const 
		{
			return { tile_size_x, tile_size_z };
		}

		std::pair<uint64, uint64> TileCounts() const
		{
			return { tile_count_x, tile_count_z };
		}

	private:
		std::vector<TexturedNormalVertex> terrain_vertices;
		float tile_size_x;
		float tile_size_z;
		uint64 tile_count_x;
		uint64 tile_count_z;
		DirectX::XMFLOAT3 offset;

	private:

		uint64 GetIndex(uint64 x_, uint64 z_) const
		{
			return x_ + z_ * (tile_count_x + 1);
		}

		bool CheckIndex(uint64 i) const
		{
			return i >= 0 && i < terrain_vertices.size();
		}
	};
}