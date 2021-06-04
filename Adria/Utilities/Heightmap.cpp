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


	Heightmap::Heightmap(noise_desc const& desc)
	{
		FastNoiseLite noise{};
		noise.SetFractalType(GetFractalType(desc.fractal_type));
		noise.SetSeed(desc.seed);
		noise.SetNoiseType(GetNoiseType(desc.noise_type));
		noise.SetFractalOctaves(desc.octaves);
		noise.SetFractalLacunarity(desc.lacunarity);
		noise.SetFractalGain(desc.persistence);

		hm.resize(desc.depth);

		for (u64 y = 0; y < desc.depth; y++)
		{

			hm[y].resize(desc.width);
			for (u64 x = 0; x < desc.width; x++)
			{

				f32 xf = (x / (f32)desc.width) * desc.noise_size;
				f32 zf = (y / (f32)desc.depth) * desc.noise_size;

				f32 total = noise.GetNoise(xf, zf);

				total = (total + 1.0f) / 2;

				hm[y][x] = total * desc.max_height;
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