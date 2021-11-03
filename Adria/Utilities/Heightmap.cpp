#include "Heightmap.h"
#include "Cpp/FastNoiseLite.h"
#include "../Utilities/Image.h"

namespace adria
{
	
	static constexpr FastNoiseLite::NoiseType GetNoiseType(ENoiseType type)
	{
		switch (type)
		{
		case ENoiseType::OpenSimplex2:
			return FastNoiseLite::NoiseType_OpenSimplex2;
		case ENoiseType::OpenSimplex2S:
			return FastNoiseLite::NoiseType_OpenSimplex2S;
		case ENoiseType::Cellular:
			return FastNoiseLite::NoiseType_OpenSimplex2;
		case ENoiseType::ValueCubic:
			return FastNoiseLite::NoiseType_ValueCubic;
		case ENoiseType::Value:
			return FastNoiseLite::NoiseType_Value;
		case ENoiseType::Perlin:
		default:
			return FastNoiseLite::NoiseType_Perlin;
		}

		return FastNoiseLite::NoiseType_Perlin;
	}
	static constexpr FastNoiseLite::FractalType GetFractalType(EFractalType type)
	{
		switch (type)
		{
		case EFractalType::FBM:
			return FastNoiseLite::FractalType_FBm;
		case EFractalType::Ridged:
			return FastNoiseLite::FractalType_Ridged;
		case EFractalType::PingPong:
			return FastNoiseLite::FractalType_PingPong;
		case EFractalType::None:
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
	Heightmap::Heightmap(std::string_view heightmap_path, u32 max_height)
	{
		Image img(heightmap_path, 1);

		u8 const* image_data = img.Data<u8>();

		hm.resize(img.Height());
		for (u64 z = 0; z < img.Height(); ++z)
		{
			hm[z].resize(img.Width());
			for (u64 x = 0; x < img.Width(); ++x)
			{
				hm[z][x] = image_data[z * img.Width() + x] / 255.0f * max_height;
			}
		}
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