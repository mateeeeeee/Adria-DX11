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
		U32 width;
		U32 depth;
		U32 max_height;
		EFractalType fractal_type = EFractalType::None;
		ENoiseType noise_type = ENoiseType::Perlin;
		I32 seed = 1337;
		F32 frequency = 0.01f;
		F32 persistence = 0.5f;
		F32 lacunarity = 2.0f;
		I32 octaves = 3;
		F32 noise_scale = 10;
	};


	struct thermal_erosion_desc_t
	{
		I32 iterations;
		F32 c;
		F32 talus;
	};

	
	struct hydraulic_erosion_desc_t
	{
		I32 iterations;
		I32 drops;
		F32 carrying_capacity;
		F32 deposition_speed;
	};

	class Heightmap
	{
	public:
		
		Heightmap(noise_desc_t const& desc);

		Heightmap(std::string_view heightmap_path, U32 max_height);

		F32 HeightAt(u64 x, u64 z) const;

		u64 Width() const;

		u64 Depth() const;

		void ApplyThermalErosion(thermal_erosion_desc_t const& desc);

		void ApplyHydraulicErosion(hydraulic_erosion_desc_t const& desc);

	private:
		std::vector<std::vector<F32>> hm;
	};
}

