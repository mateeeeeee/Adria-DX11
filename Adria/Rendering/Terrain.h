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
		Terrain(std::vector<TexturedNormalVertex> const& terrain_vertices, f32 tx, f32 tz, u64 xcount, u64 zcount);

		f32 HeightAt(f32 x, f32 z) const;
		
		DirectX::XMFLOAT3 NormalAt(f32 x, f32 z) const;

		std::pair<f32, f32> TileSizes() const 
		{
			return { tile_size_x, tile_size_z };
		}

		std::pair<u64, u64> TileCounts() const
		{
			return { tile_count_x, tile_count_z };
		}

	private:
		std::vector<TexturedNormalVertex> terrain_vertices;
		f32 tile_size_x;
		f32 tile_size_z;
		u64 tile_count_x;
		u64 tile_count_z;
		DirectX::XMFLOAT3 offset;

	private:

		u64 GetIndex(u64 x_, u64 z_) const
		{
			return x_ + z_ * (tile_count_x + 1);
		}

		bool CheckIndex(u64 i) const
		{
			return i >= 0 && i < terrain_vertices.size();
		}
	};
}