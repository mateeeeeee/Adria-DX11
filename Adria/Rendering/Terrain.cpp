#include "Terrain.h"
#include <cmath>

using namespace DirectX;

namespace adria
{

	Terrain::Terrain(std::vector<TexturedNormalVertex> const& terrain_vertices, float tx, float tz, uint64 xcount, uint64 zcount) : terrain_vertices(terrain_vertices), tile_size_x(tx), tile_size_z(tz),
		tile_count_x(xcount), tile_count_z(zcount), offset()
	{}

	float Terrain::HeightAt(float x, float z) const
	{
		uint64 x_1 = (uint64)(x / tile_size_x);
		uint64 x_2 = (uint64)((x + tile_size_x) / tile_size_x);

		uint64 z_1 = (uint64)(z / tile_size_z);
		uint64 z_2 = (uint64)((z + tile_size_z) / tile_size_z);

		uint64 index1 = GetIndex(x_1, z_1);
		float h1 = CheckIndex(index1) ? terrain_vertices[index1].position.y : 0.0f;

		uint64 index2 = GetIndex(x_2, z_1);
		float h2 = CheckIndex(index2) ? terrain_vertices[index2].position.y : 0.0f;

		float alpha_x = (x - x_1 * tile_size_x) / ((x_2 - x_1) * tile_size_x);
		float h_interpolated1 = std::lerp(h1, h2, alpha_x);

		uint64 index3 = GetIndex(x_1, z_2);
		float h3 = CheckIndex(index3) ? terrain_vertices[index3].position.y : 0.0f;

		uint64 index4 = GetIndex(x_2, z_2);
		float h4 = CheckIndex(index4) ? terrain_vertices[index4].position.y : 0.0f;

		float h_interpolated2 = std::lerp(h3, h4, alpha_x);

		float alpha_z = (z - z_1 * tile_size_z) / ((z_2 - z_1) * tile_size_z);

		return std::lerp(h_interpolated1, h_interpolated2, alpha_z);
	}

	Vector3 Terrain::NormalAt(float x, float z) const
	{
		uint64 x_1 = (uint64)(x / tile_size_x);
		uint64 x_2 = (uint64)((x + tile_size_x) / tile_size_x);

		uint64 z_1 = (uint64)(z / tile_size_z);
		uint64 z_2 = (uint64)((z + tile_size_z) / tile_size_z);

		uint64 index1 = GetIndex(x_1, z_1);
		Vector3 n1 = CheckIndex(index1) ? terrain_vertices[index1].normal : Vector3(0.0f, 1.0f, 0.0f);

		uint64 index2 = GetIndex(x_2, z_1);
		Vector3 n2 = CheckIndex(index2) ? terrain_vertices[index2].normal : Vector3(0.0f, 1.0f, 0.0f);

		float alpha_x = (x - x_1 * tile_size_x) / ((x_2 - x_1) * tile_size_x);

		Vector3 n_interpolated1 = Vector3::Lerp(n1, n2, alpha_x);

		uint64 index3 = GetIndex(x_1, z_2);
		Vector3 n3 = CheckIndex(index3) ? terrain_vertices[index3].normal : Vector3(0.0f, 1.0f, 0.0f);

		uint64 index4 = GetIndex(x_2, z_2);
		Vector3 n4 = CheckIndex(index4) ? terrain_vertices[index4].normal : Vector3(0.0f, 1.0f, 0.0f);

		Vector3 n_interpolated2 = Vector3::Lerp(n3, n4, alpha_x);

		float alpha_z = (z - z_1 * tile_size_z) / ((z_2 - z_1) * tile_size_z);

		Vector3 normal = Vector3::Lerp(n_interpolated1, n_interpolated2, alpha_z);
		return normal;
	}

}

