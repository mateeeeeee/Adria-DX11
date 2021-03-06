#pragma once
#include <vector>
#include <DirectXMath.h>
#include "../Core/Definitions.h"
#include "../Graphics/VertexTypes.h"

namespace adria
{
	class Terrain
	{
	public:
		Terrain(std::vector<TexturedNormalVertex> const& terrain_vertices, float32 tx, float32 tz, uint64 xcount, uint64 zcount);

		float32 HeightAt(float32 x, float32 z) const;
		
		DirectX::XMFLOAT3 NormalAt(float32 x, float32 z) const;

		std::pair<float32, float32> TileSizes() const 
		{
			return { tile_size_x, tile_size_z };
		}

		std::pair<uint64, uint64> TileCounts() const
		{
			return { tile_count_x, tile_count_z };
		}

	private:
		std::vector<TexturedNormalVertex> terrain_vertices;
		float32 tile_size_x;
		float32 tile_size_z;
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