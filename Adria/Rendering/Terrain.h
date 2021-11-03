#pragma once
#include <cmath>
#include "../Utilities/Heightmap.h"

namespace adria
{
	class Terrain
	{
	public:
		Terrain(std::vector<TexturedNormalVertex>& terrain_vertices, u64 tx, u64 tz, u64 xcount, u64 zcount)
			: terrain_vertices(terrain_vertices), tile_size_x(tx), tile_size_z(tz),
			  tile_count_x(xcount), tile_count_z(zcount), offset()
		{
		}

		f32 HeightAt(f32 x, f32 z) const //do bilinear interpolation, use offset
		{
			u32 x_ = (u32)std::round(x / tile_size_x);
			u32 z_ = (u32)std::round(z / tile_size_z);
			u32 i = x_ + z_ * (tile_count_x + 1);
			
			if (i < 0 || i >= terrain_vertices.size()) return 0.0f;
			else return terrain_vertices[i].position.y;
		}
		
		DirectX::XMFLOAT3 NormalAt(f32 x, f32 z) const
		{
			u32 x_ = (u32)std::round(x / tile_size_x);
			u32 z_ = (u32)std::round(z / tile_size_z);
			u32 i = x_ + z_ * (tile_count_x + 1);
			if (i < 0 || i >= terrain_vertices.size()) return DirectX::XMFLOAT3{0.0f, 1.0f, 0.0f};
			else return terrain_vertices[i].normal;
		}

	private:
		std::vector<TexturedNormalVertex> terrain_vertices;
		u64 tile_size_x;
		u64 tile_size_z;
		u64 tile_count_x;
		u64 tile_count_z;
		DirectX::XMFLOAT3 offset;
	};
}