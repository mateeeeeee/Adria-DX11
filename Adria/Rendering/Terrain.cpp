#include "Terrain.h"
#include <cmath>

using namespace DirectX;

namespace adria
{

	Terrain::Terrain(std::vector<TexturedNormalVertex> const& terrain_vertices, Float tx, Float tz, Uint64 xcount, Uint64 zcount) : terrain_vertices(terrain_vertices), tile_size_x(tx), tile_size_z(tz),
		tile_count_x(xcount), tile_count_z(zcount), offset()
	{}

	Float Terrain::HeightAt(Float x, Float z) const
	{
		Uint64 x_1 = (Uint64)(x / tile_size_x);
		Uint64 x_2 = (Uint64)((x + tile_size_x) / tile_size_x);

		Uint64 z_1 = (Uint64)(z / tile_size_z);
		Uint64 z_2 = (Uint64)((z + tile_size_z) / tile_size_z);

		Uint64 index1 = GetIndex(x_1, z_1);
		Float h1 = CheckIndex(index1) ? terrain_vertices[index1].position.y : 0.0f;

		Uint64 index2 = GetIndex(x_2, z_1);
		Float h2 = CheckIndex(index2) ? terrain_vertices[index2].position.y : 0.0f;

		Float alpha_x = (x - x_1 * tile_size_x) / ((x_2 - x_1) * tile_size_x);
		Float h_interpolated1 = std::lerp(h1, h2, alpha_x);

		Uint64 index3 = GetIndex(x_1, z_2);
		Float h3 = CheckIndex(index3) ? terrain_vertices[index3].position.y : 0.0f;

		Uint64 index4 = GetIndex(x_2, z_2);
		Float h4 = CheckIndex(index4) ? terrain_vertices[index4].position.y : 0.0f;

		Float h_interpolated2 = std::lerp(h3, h4, alpha_x);

		Float alpha_z = (z - z_1 * tile_size_z) / ((z_2 - z_1) * tile_size_z);

		return std::lerp(h_interpolated1, h_interpolated2, alpha_z);
	}

	Vector3 Terrain::NormalAt(Float x, Float z) const
	{
		Uint64 x_1 = (Uint64)(x / tile_size_x);
		Uint64 x_2 = (Uint64)((x + tile_size_x) / tile_size_x);

		Uint64 z_1 = (Uint64)(z / tile_size_z);
		Uint64 z_2 = (Uint64)((z + tile_size_z) / tile_size_z);

		Uint64 index1 = GetIndex(x_1, z_1);
		Vector3 n1 = CheckIndex(index1) ? terrain_vertices[index1].normal : Vector3(0.0f, 1.0f, 0.0f);

		Uint64 index2 = GetIndex(x_2, z_1);
		Vector3 n2 = CheckIndex(index2) ? terrain_vertices[index2].normal : Vector3(0.0f, 1.0f, 0.0f);

		Float alpha_x = (x - x_1 * tile_size_x) / ((x_2 - x_1) * tile_size_x);

		Vector3 n_interpolated1 = Vector3::Lerp(n1, n2, alpha_x);

		Uint64 index3 = GetIndex(x_1, z_2);
		Vector3 n3 = CheckIndex(index3) ? terrain_vertices[index3].normal : Vector3(0.0f, 1.0f, 0.0f);

		Uint64 index4 = GetIndex(x_2, z_2);
		Vector3 n4 = CheckIndex(index4) ? terrain_vertices[index4].normal : Vector3(0.0f, 1.0f, 0.0f);

		Vector3 n_interpolated2 = Vector3::Lerp(n3, n4, alpha_x);

		Float alpha_z = (z - z_1 * tile_size_z) / ((z_2 - z_1) * tile_size_z);

		Vector3 normal = Vector3::Lerp(n_interpolated1, n_interpolated2, alpha_z);
		return normal;
	}

}

