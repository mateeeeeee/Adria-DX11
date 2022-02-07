#include "Terrain.h"
#include <cmath>

using namespace DirectX;

namespace adria
{

	Terrain::Terrain(std::vector<TexturedNormalVertex> const& terrain_vertices, float32 tx, float32 tz, uint64 xcount, uint64 zcount) : terrain_vertices(terrain_vertices), tile_size_x(tx), tile_size_z(tz),
		tile_count_x(xcount), tile_count_z(zcount), offset()
	{}

	float32 Terrain::HeightAt(float32 x, float32 z) const
	{
		uint64 x_1 = (uint64)(x / tile_size_x);
		uint64 x_2 = (uint64)((x + tile_size_x) / tile_size_x);

		uint64 z_1 = (uint64)(z / tile_size_z);
		uint64 z_2 = (uint64)((z + tile_size_z) / tile_size_z);

		uint64 index1 = GetIndex(x_1, z_1);
		float32 h1 = CheckIndex(index1) ? terrain_vertices[index1].position.y : 0.0f;

		uint64 index2 = GetIndex(x_2, z_1);
		float32 h2 = CheckIndex(index2) ? terrain_vertices[index2].position.y : 0.0f;

		float32 alpha_x = (x - x_1 * tile_size_x) / ((x_2 - x_1) * tile_size_x);
		float32 h_interpolated1 = std::lerp(h1, h2, alpha_x);

		uint64 index3 = GetIndex(x_1, z_2);
		float32 h3 = CheckIndex(index3) ? terrain_vertices[index3].position.y : 0.0f;

		uint64 index4 = GetIndex(x_2, z_2);
		float32 h4 = CheckIndex(index4) ? terrain_vertices[index4].position.y : 0.0f;

		float32 h_interpolated2 = std::lerp(h3, h4, alpha_x);

		float32 alpha_z = (z - z_1 * tile_size_z) / ((z_2 - z_1) * tile_size_z);

		return std::lerp(h_interpolated1, h_interpolated2, alpha_z);
	}

	XMFLOAT3 Terrain::NormalAt(float32 x, float32 z) const
	{
		uint64 x_1 = (uint64)(x / tile_size_x);
		uint64 x_2 = (uint64)((x + tile_size_x) / tile_size_x);

		uint64 z_1 = (uint64)(z / tile_size_z);
		uint64 z_2 = (uint64)((z + tile_size_z) / tile_size_z);

		uint64 index1 = GetIndex(x_1, z_1);
		XMFLOAT3 n1 = CheckIndex(index1) ? terrain_vertices[index1].normal : XMFLOAT3(0.0f, 1.0f, 0.0f);

		uint64 index2 = GetIndex(x_2, z_1);
		XMFLOAT3 n2 = CheckIndex(index2) ? terrain_vertices[index2].normal : XMFLOAT3(0.0f, 1.0f, 0.0f);

		float32 alpha_x = (x - x_1 * tile_size_x) / ((x_2 - x_1) * tile_size_x);

		XMVECTOR n_interpolated1 = XMVectorLerp(
			XMLoadFloat3(&n1), XMLoadFloat3(&n2), alpha_x
		);

		uint64 index3 = GetIndex(x_1, z_2);
		XMFLOAT3 n3 = CheckIndex(index3) ? terrain_vertices[index3].normal : XMFLOAT3(0.0f, 1.0f, 0.0f);

		uint64 index4 = GetIndex(x_2, z_2);
		XMFLOAT3 n4 = CheckIndex(index4) ? terrain_vertices[index4].normal : XMFLOAT3(0.0f, 1.0f, 0.0f);

		XMVECTOR n_interpolated2 = XMVectorLerp(
			XMLoadFloat3(&n3), XMLoadFloat3(&n4), alpha_x
		);

		float32 alpha_z = (z - z_1 * tile_size_z) / ((z_2 - z_1) * tile_size_z);

		XMVECTOR n_final = XMVectorLerp(
			n_interpolated1, n_interpolated2, alpha_z
		);

		XMFLOAT3 normal;
		XMStoreFloat3(&normal, n_final);

		return normal;
	}

}

