#pragma once
#include "../Core/CoreTypes.h"
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
	struct NoiseDesc
	{
		uint32 width;
		uint32 depth;
		uint32 max_height;
		EFractalType fractal_type = EFractalType::None;
		ENoiseType noise_type = ENoiseType::Perlin;
		int32 seed = 1337;
		float32 frequency = 0.01f;
		float32 persistence = 0.5f;
		float32 lacunarity = 2.0f;
		int32 octaves = 3;
		float32 noise_scale = 10;
	};


	struct ThermalErosionDesc
	{
		int32 iterations;
		float32 c;
		float32 talus;
	};

	
	struct HydraulicErosionDesc
	{
		int32 iterations;
		int32 drops;
		float32 carrying_capacity;
		float32 deposition_speed;
	};

	class Heightmap
	{
	public:
		
		Heightmap(NoiseDesc const& desc);

		Heightmap(std::string_view heightmap_path, uint32 max_height);

		float32 HeightAt(uint64 x, uint64 z) const;

		uint64 Width() const;

		uint64 Depth() const;

		void ApplyThermalErosion(ThermalErosionDesc const& desc);

		void ApplyHydraulicErosion(HydraulicErosionDesc const& desc);

	private:
		std::vector<std::vector<float32>> hm;
	};
}

