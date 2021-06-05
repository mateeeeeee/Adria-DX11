#include "Heightmap.h"
//move to cpp
#include "Cpp/FastNoiseLite.h"

namespace adria
{
	
	constexpr FastNoiseLite::NoiseType GetNoiseType(NoiseType type)
	{
		switch (type)
		{
		case NoiseType::eOpenSimplex2:
			return FastNoiseLite::NoiseType_OpenSimplex2;
		case NoiseType::eOpenSimplex2S:
			return FastNoiseLite::NoiseType_OpenSimplex2S;
		case NoiseType::eCellular:
			return FastNoiseLite::NoiseType_OpenSimplex2;
		case NoiseType::eValueCubic:
			return FastNoiseLite::NoiseType_ValueCubic;
		case NoiseType::eValue:
			return FastNoiseLite::NoiseType_Value;
		case NoiseType::ePerlin:
		default:
			return FastNoiseLite::NoiseType_Perlin;
		}

		return FastNoiseLite::NoiseType_Perlin;
	}
	constexpr FastNoiseLite::FractalType GetFractalType(FractalType type)
	{
		switch (type)
		{
		case FractalType::eFBM:
			return FastNoiseLite::FractalType_FBm;
		case FractalType::eRidged:
			return FastNoiseLite::FractalType_Ridged;
		case FractalType::ePingPong:
			return FastNoiseLite::FractalType_PingPong;
		case FractalType::eNone:
		default:
			return FastNoiseLite::FractalType_None;
		}

		return FastNoiseLite::FractalType_None;
	}


	Heightmap::Heightmap(noise_desc_t const& desc)
	{
		FastNoiseLite noise{};
		noise.SetFractalType(GetFractalType(desc.fractal_type));
		noise.SetSeed(desc.seed);
		noise.SetNoiseType(GetNoiseType(desc.noise_type));
		noise.SetFractalOctaves(desc.octaves);
		noise.SetFractalLacunarity(desc.lacunarity);
		noise.SetFractalGain(desc.persistence);
		noise.SetFrequency(0.1f);
		hm.resize(desc.depth);

		for (u32 z = 0; z < desc.depth; z++)
		{

			hm[z].resize(desc.width);
			for (u32 x = 0; x < desc.width; x++)
			{

				f32 xf = x * desc.noise_scale / desc.width; // - desc.width / 2;
				f32 zf = z * desc.noise_scale / desc.depth; // - desc.depth / 2;

				f32 total = noise.GetNoise(xf, zf);

				hm[z][x] = total * desc.max_height;
			}
		}
	}
	Heightmap::Heightmap(std::string_view heightmap_path)
	{
		//todo
	}
	f32 Heightmap::HeightAt(u64 x, u64 z)
	{
		return hm[z][x];
	}
	u64 Heightmap::Width() const
	{
		return hm[0].size();
	}
	u64 Heightmap::Depth() const
	{
		return hm.size();
	}
}