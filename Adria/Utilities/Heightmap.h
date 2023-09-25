#pragma once
#include <vector>
#include <string_view>

namespace adria
{
	enum class NoiseType
	{
		OpenSimplex2,
		OpenSimplex2S,
		Cellular,
		Perlin,
		ValueCubic,
		Value
	};
	enum class FractalType
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
		FractalType fractal_type = FractalType::None;
		NoiseType noise_type = NoiseType::Perlin;
		int32 seed = 1337;
		float frequency = 0.01f;
		float persistence = 0.5f;
		float lacunarity = 2.0f;
		int32 octaves = 3;
		float noise_scale = 10;
	};


	struct ThermalErosionDesc
	{
		int32 iterations;
		float c;
		float talus;
	};

	
	struct HydraulicErosionDesc
	{
		int32 iterations;
		int32 drops;
		float carrying_capacity;
		float deposition_speed;
	};

	class Heightmap
	{
	public:
		
		explicit Heightmap(NoiseDesc const& desc);
		Heightmap(std::string_view heightmap_path, uint32 max_height);

		float HeightAt(uint64 x, uint64 z) const;
		uint64 Width() const;
		uint64 Depth() const;

		void ApplyThermalErosion(ThermalErosionDesc const& desc);
		void ApplyHydraulicErosion(HydraulicErosionDesc const& desc);

	private:
		std::vector<std::vector<float>> hm;
	};
}

