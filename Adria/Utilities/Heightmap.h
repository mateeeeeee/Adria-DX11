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
		Uint32 width;
		Uint32 depth;
		Uint32 max_height;
		FractalType fractal_type = FractalType::None;
		NoiseType noise_type = NoiseType::Perlin;
		Int32 seed = 1337;
		Float frequency = 0.01f;
		Float persistence = 0.5f;
		Float lacunarity = 2.0f;
		Int32 octaves = 3;
		Float noise_scale = 10;
	};


	struct ThermalErosionDesc
	{
		Int32 iterations;
		Float c;
		Float talus;
	};

	
	struct HydraulicErosionDesc
	{
		Int32 iterations;
		Int32 drops;
		Float carrying_capacity;
		Float deposition_speed;
	};

	class Heightmap
	{
	public:
		
		explicit Heightmap(NoiseDesc const& desc);
		Heightmap(std::string_view heightmap_path, Uint32 max_height);

		Float HeightAt(Uint64 x, Uint64 z) const;
		Uint64 Width() const;
		Uint64 Depth() const;

		void ApplyThermalErosion(ThermalErosionDesc const& desc);
		void ApplyHydraulicErosion(HydraulicErosionDesc const& desc);

	private:
		std::vector<std::vector<Float>> hm;
	};
}

