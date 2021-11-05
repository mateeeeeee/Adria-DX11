#pragma once
#include <cmath>
#include "../Utilities/Heightmap.h"

namespace adria
{
	class Terrain
	{
	public:
		Terrain(std::vector<TexturedNormalVertex> const& terrain_vertices, f32 tx, f32 tz, u64 xcount, u64 zcount)
			: terrain_vertices(terrain_vertices), tile_size_x(tx), tile_size_z(tz),
			  tile_count_x(xcount), tile_count_z(zcount), offset()
		{
		}

		f32 HeightAt(f32 x, f32 z) const 
		{
			u64 x_1 = (u64)(x / tile_size_x);
			u64 x_2 = (u64)((x + tile_size_x) / tile_size_x);
			
			u64 z_1 = (u64)(z / tile_size_z);
			u64 z_2 = (u64)((z + tile_size_z) / tile_size_z);

			u64 index1 = GetIndex(x_1, z_1);
			f32 h1 = CheckIndex(index1) ? terrain_vertices[index1].position.y : 0.0f;

			u64 index2 = GetIndex(x_2, z_1);
			f32 h2 = CheckIndex(index2) ? terrain_vertices[index2].position.y : 0.0f;

			f32 alpha_x = (x - x_1 * tile_size_x) / ((x_2 - x_1) * tile_size_x);
			f32 h_interpolated1 = std::lerp(h1, h2, alpha_x);

			u64 index3 = GetIndex(x_1, z_2);
			f32 h3 = CheckIndex(index3) ? terrain_vertices[index3].position.y : 0.0f;

			u64 index4 = GetIndex(x_2, z_2);
			f32 h4 = CheckIndex(index4) ? terrain_vertices[index4].position.y : 0.0f;

			f32 h_interpolated2 = std::lerp(h3, h4, alpha_x);

			f32 alpha_z = (z - z_1 * tile_size_z) / ((z_2 - z_1) * tile_size_z);

			return std::lerp(h_interpolated1, h_interpolated2, alpha_z);
		}
		
		DirectX::XMFLOAT3 NormalAt(f32 x, f32 z) const
		{
			u64 x_1 = (u64)(x / tile_size_x);
			u64 x_2 = (u64)((x + tile_size_x) / tile_size_x);

			u64 z_1 = (u64)(z / tile_size_z);
			u64 z_2 = (u64)((z + tile_size_z) / tile_size_z);

			u64 index1 = GetIndex(x_1, z_1);
			DirectX::XMFLOAT3 n1 = CheckIndex(index1) ? terrain_vertices[index1].normal : DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

			u64 index2 = GetIndex(x_2, z_1);
			DirectX::XMFLOAT3 n2 = CheckIndex(index2) ? terrain_vertices[index2].normal : DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
			
			f32 alpha_x = (x - x_1 * tile_size_x) / ((x_2 - x_1) * tile_size_x);

			DirectX::XMVECTOR n_interpolated1 = DirectX::XMVectorLerp(
				DirectX::XMLoadFloat3(&n1), DirectX::XMLoadFloat3(&n2), alpha_x
			);

			u64 index3 = GetIndex(x_1, z_2);
			DirectX::XMFLOAT3 n3 = CheckIndex(index3) ? terrain_vertices[index3].normal : DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

			u64 index4 = GetIndex(x_2, z_2);
			DirectX::XMFLOAT3 n4 = CheckIndex(index4) ? terrain_vertices[index4].normal : DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

			DirectX::XMVECTOR n_interpolated2 = DirectX::XMVectorLerp(
				DirectX::XMLoadFloat3(&n3), DirectX::XMLoadFloat3(&n4), alpha_x
			);

			f32 alpha_z = (z - z_1 * tile_size_z) / ((z_2 - z_1) * tile_size_z);

			DirectX::XMVECTOR n_final = DirectX::XMVectorLerp(
				n_interpolated1, n_interpolated2, alpha_z
			);

			DirectX::XMFLOAT3 normal;
			DirectX::XMStoreFloat3(&normal, n_final);
			
			return normal;
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