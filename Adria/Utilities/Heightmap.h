#pragma once
#include "../Core/Definitions.h"
#include <vector>
#include <string_view>

namespace adria
{
	enum class ENoiseType
	{
		OpenSimplex2,
		OpenSimplex2S,
		Cellular,
		Perlin,
		ValueCubic,
		Value
	};
	enum class EFractalType
	{
		None,
		FBM,
		Ridged,
		PingPong
	};
	struct noise_desc_t
	{
		u32 width;
		u32 depth;
		u32 max_height;
		EFractalType fractal_type = EFractalType::None;
		ENoiseType noise_type = ENoiseType::Perlin;
		i32 seed = 1337;
		f32 frequency = 0.01f;
		f32 persistence = 0.5f;
		f32 lacunarity = 2.0f;
		i32 octaves = 3;
		f32 noise_scale = 10;
	};

	struct thermal_erosion_desc_t
	{
		i32 iterations;
		f32 c;
		f32 talus;
	};

	struct hydraulic_erosion_desc_t
	{
		i32 iterations;
		i32 drops;
		f32 carrying_capacity;
		f32 deposition_speed;
	};

	class Heightmap
	{
	public:
		
		Heightmap(noise_desc_t const& desc);

		Heightmap(std::string_view heightmap_path, u32 max_height);

		f32 HeightAt(u64 x, u64 z) const;

		u64 Width() const;

		u64 Depth() const;

		void ApplyThermalErosion(thermal_erosion_desc_t const& desc);

		void ApplyHydraulicErosion(hydraulic_erosion_desc_t const& desc);

	private:
		std::vector<std::vector<f32>> hm;
	};
}

