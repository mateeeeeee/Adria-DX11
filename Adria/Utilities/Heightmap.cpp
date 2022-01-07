#include "Heightmap.h"
#include "Cpp/FastNoiseLite.h"
#include "Image.h"
#include "Random.h"
#include <algorithm>
#include <numeric>

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
	static float DepositSediment(float c, float max_diff, float talus, float distance, float total_diff)
	{
		return (distance > talus) ? (c * (max_diff - talus) * (distance / total_diff)) : 0.0f;
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
		noise.SetFrequency(desc.frequency);
		hm.resize(desc.depth);

		F32 max_height_achieved = std::numeric_limits<F32>::min();
		F32 min_height_achieved = std::numeric_limits<F32>::max();

		for (U32 z = 0; z < desc.depth; z++)
		{
			hm[z].resize(desc.width);
			for (U32 x = 0; x < desc.width; x++)
			{
				F32 xf = x * desc.noise_scale / desc.width;
				F32 zf = z * desc.noise_scale / desc.depth;

				F32 height = noise.GetNoise(xf, zf) * desc.max_height;
				if (height > max_height_achieved) max_height_achieved = height;
				if (height < min_height_achieved) min_height_achieved = height;
				hm[z][x] = height;
			}
		}

		auto scale = [=](F32 h) -> F32
		{
			return (h - min_height_achieved) / (max_height_achieved - min_height_achieved)
				* 2 * desc.max_height - desc.max_height;
		};

		for (U32 z = 0; z < desc.depth; z++)
		{
			for (U32 x = 0; x < desc.width; x++)
			{
				hm[z][x] = scale(hm[z][x]);
			}
		}
	}
	Heightmap::Heightmap(std::string_view heightmap_path, U32 max_height)
	{
		Image img(heightmap_path, 1);

		U8 const* image_data = img.Data<U8>();

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
	F32 Heightmap::HeightAt(u64 x, u64 z) const
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

	void Heightmap::ApplyThermalErosion(thermal_erosion_desc_t const& desc)
	{
		F32 v1, v2, v3, v4, v5, v6, v7, v8, v9;	
		F32 d1, d2, d3, d4, d5, d6, d7, d8;		
		F32 talus = desc.talus;//4.0f / resolution
		F32 c = desc.c; //0.5f;					

		for (size_t k = 0; k < desc.iterations; k++)
		{
			for (size_t j = 0; j < hm.size(); j++)
			{
				for (size_t i = 0; i < hm[j].size(); i++)
				{

					F32 max_diff = 0.0f;
					F32 total_diff = 0.0f;

					d1 = 0.0f;
					d2 = 0.0f;
					d3 = 0.0f;
					d4 = 0.0f;
					d5 = 0.0f;
					d6 = 0.0f;
					d7 = 0.0f;
					d8 = 0.0f;

					v2 = hm[j][i];

					if (i == 0 && j == 0)
					{
						v3 = hm[j][i + 1l];
						v5 = hm[j + 1l][i];
						v6 = hm[j + 1l][i + 1l];

						d2 = v2 - v3;
						d4 = v2 - v5;
						d5 = v2 - v6;
					}
					else if (i == hm[j].size() - 1 && j == hm.size() - 1)
					{
						v1 = hm[j][i - 1l];
						v7 = hm[j - 1l][i - 1l];
						v8 = hm[j - 1l][i];

						d1 = v2 - v1;
						d6 = v2 - v7;
						d7 = v2 - v8;
					}
					else if (i == 0 && j == hm.size() - 1)
					{
						v3 = hm[j][i + 1l];
						v8 = hm[j - 1l][i];
						v9 = hm[j - 1l][i + 1l];

						d2 = v2 - v3;
						d7 = v2 - v8;
						d8 = v2 - v9;
					}
					else if (i == hm[j].size() - 1 && j == 0)
					{
						v1 = hm[j][i - 1l];
						v4 = hm[j + 1l][i - 1l];
						v5 = hm[j + 1l][i];

						d1 = v2 - v1;
						d3 = v2 - v4;
						d4 = v2 - v5;
					}
					else if (j == 0)
					{
						v1 = hm[j][i - 1l];
						v3 = hm[j][i + 1l];
						v4 = hm[j + 1l][i - 1l];
						v5 = hm[j + 1l][i];
						v6 = hm[j + 1l][i + 1l];

						d1 = v2 - v1;
						d2 = v2 - v3;
						d3 = v2 - v4;
						d4 = v2 - v5;
						d5 = v2 - v6;
					}
					else if (j == hm.size() - 1)
					{
						v1 = hm[j][i - 1l];
						v3 = hm[j][i + 1l];
						v7 = hm[j - 1l][i - 1l];
						v8 = hm[j - 1l][i];
						v9 = hm[j - 1l][i + 1l];

						d1 = v2 - v1;
						d2 = v2 - v3;
						d6 = v2 - v7;
						d7 = v2 - v8;
						d8 = v2 - v9;
					}
					else if (i == 0)
					{
						v3 = hm[j][i + 1l];
						v5 = hm[j + 1l][i];
						v6 = hm[j + 1l][i + 1l];
						v8 = hm[j - 1l][i];
						v9 = hm[j - 1l][i + 1l];

						d2 = v2 - v3;
						d4 = v2 - v5;
						d5 = v2 - v6;
						d7 = v2 - v8;
						d8 = v2 - v9;
					}
					else if (i == hm[j].size() - 1)
					{
						v1 = hm[j][i - 1l];
						v4 = hm[j + 1l][i - 1l];
						v5 = hm[j + 1l][i];
						v7 = hm[j - 1l][i - 1l];
						v8 = hm[j - 1l][i];

						d1 = v2 - v1;
						d3 = v2 - v4;
						d4 = v2 - v5;
						d6 = v2 - v7;
						d7 = v2 - v8;
					}
					else
					{
						v1 = hm[j][i - 1l];
						v3 = hm[j][i + 1l];
						v4 = hm[j + 1l][i - 1l];
						v5 = hm[j + 1l][i];
						v6 = hm[j + 1l][i + 1l];
						v7 = hm[j - 1l][i - 1l];
						v8 = hm[j - 1l][i];
						v9 = hm[j - 1l][i + 1l];

						d1 = v2 - v1;
						d2 = v2 - v3;
						d3 = v2 - v4;
						d4 = v2 - v5;
						d5 = v2 - v6;
						d6 = v2 - v7;
						d7 = v2 - v8;
						d8 = v2 - v9;
					}

					F32 d_array[] = { d1, d2, d3, d4, d5, d6, d7, d8 };
					for (F32 d : d_array)
					{
						if (d > talus)
						{
							total_diff += d;
							if (d > max_diff) max_diff = d;
						}
					}

					if (i == 0 && j == 0)
					{
						hm[j][i + 1l] += DepositSediment(c, max_diff, talus, d2, total_diff);
						hm[j + 1l][i] += DepositSediment(c, max_diff, talus, d4, total_diff);
						hm[j + 1l][i + 1l] += DepositSediment(c, max_diff, talus, d5, total_diff);
					}
					else if (i == hm[j].size() - 1 && j == hm.size() - 1)
					{
						hm[j][i - 1l] += DepositSediment(c, max_diff, talus, d1, total_diff);
						hm[j - 1l][i - 1l] += DepositSediment(c, max_diff, talus, d6, total_diff);
						hm[j - 1l][i] += DepositSediment(c, max_diff, talus, d7, total_diff);
					}
					else if (i == 0 && j == hm.size() - 1)
					{
						hm[j][i + 1l] += DepositSediment(c, max_diff, talus, d2, total_diff);
						hm[j - 1l][i] += DepositSediment(c, max_diff, talus, d7, total_diff);
						hm[j - 1l][i + 1l] += DepositSediment(c, max_diff, talus, d8, total_diff);
					}
					else if (i == hm[j].size() - 1 && j == 0)
					{
						hm[j][i - 1l] += DepositSediment(c, max_diff, talus, d1, total_diff);
						hm[j + 1l][i - 1l] += DepositSediment(c, max_diff, talus, d3, total_diff);
						hm[j + 1l][i] += DepositSediment(c, max_diff, talus, d4, total_diff);
					}
					else if (j == 0)
					{
						hm[j][i - 1l] += DepositSediment(c, max_diff, talus, d1, total_diff);
						hm[j][i + 1l] += DepositSediment(c, max_diff, talus, d2, total_diff);
						hm[j + 1l][i - 1l] += DepositSediment(c, max_diff, talus, d3, total_diff);
						hm[j + 1l][i] += DepositSediment(c, max_diff, talus, d4, total_diff);
						hm[j + 1l][i + 1l] += DepositSediment(c, max_diff, talus, d5, total_diff);
					}
					else if (j == hm.size() - 1)
					{
						hm[j][i - 1l] += DepositSediment(c, max_diff, talus, d1, total_diff);
						hm[j][i + 1l] += DepositSediment(c, max_diff, talus, d2, total_diff);
						hm[j - 1l][i - 1l] += DepositSediment(c, max_diff, talus, d6, total_diff);
						hm[j - 1l][i] += DepositSediment(c, max_diff, talus, d7, total_diff);
						hm[j - 1l][i + 1l] += DepositSediment(c, max_diff, talus, d8, total_diff);
					}
					else if (i == 0)
					{
						hm[j][i + 1l] += DepositSediment(c, max_diff, talus, d2, total_diff);
						hm[j + 1l][i] += DepositSediment(c, max_diff, talus, d4, total_diff);
						hm[j + 1l][i + 1l] += DepositSediment(c, max_diff, talus, d5, total_diff);
						hm[j - 1l][i] += DepositSediment(c, max_diff, talus, d7, total_diff);
						hm[j - 1l][i + 1l] += DepositSediment(c, max_diff, talus, d8, total_diff);
					}
					else if (i == hm[j].size() - 1)
					{
						hm[j][i - 1l] += DepositSediment(c, max_diff, talus, d1, total_diff);
						hm[j + 1l][i - 1l] += DepositSediment(c, max_diff, talus, d3, total_diff);
						hm[j + 1l][i] += DepositSediment(c, max_diff, talus, d4, total_diff);
						hm[j - 1l][i - 1l] += DepositSediment(c, max_diff, talus, d6, total_diff);
						hm[j - 1l][i] += DepositSediment(c, max_diff, talus, d7, total_diff);
					}
					else
					{
						hm[j][i - 1l] += DepositSediment(c, max_diff, talus, d1, total_diff);
						hm[j][i + 1l] += DepositSediment(c, max_diff, talus, d2, total_diff);
						hm[j + 1l][i - 1l] += DepositSediment(c, max_diff, talus, d3, total_diff);
						hm[j + 1l][i] += DepositSediment(c, max_diff, talus, d4, total_diff);
						hm[j + 1l][i + 1l] += DepositSediment(c, max_diff, talus, d5, total_diff);
						hm[j - 1l][i - 1l] += DepositSediment(c, max_diff, talus, d6, total_diff);
						hm[j - 1l][i] += DepositSediment(c, max_diff, talus, d7, total_diff);
						hm[j - 1l][i + 1l] += DepositSediment(c, max_diff, talus, d8, total_diff);
					}
				}
			}
		}
	}

	void Heightmap::ApplyHydraulicErosion(hydraulic_erosion_desc_t const& desc)
	{
		u64 drops = (u64)desc.drops;

		u64 xresolution = Width();
		u64 yresolution = Depth();

		IntRandomGenerator<u64> x_random(0, xresolution - 1);
		IntRandomGenerator<u64> y_random(0, yresolution - 1);

		for (u64 drop = 0; drop < drops; drop++)
		{
			// Get random coordinates to drop the water droplet at
			u64 X = x_random();
			u64 Y = y_random();

			F32 carryingAmount = 0.0f;
			F32 minSlope = 1.15f;

			if (hm[Y][X] > 0.0f)
			{
				for (I32 iter = 0; iter < desc.iterations; iter++)
				{
					F32 val = hm[Y][X];
					F32 left = 1000.0f;
					F32 right = 1000.0f;
					F32 up = 1000.0f;
					F32 down = 1000.0f;

					if (X == 0 && Y == 0)
					{
						right = hm[Y][X + 1];
						up = hm[Y + 1][X];
					}
					else if (X == xresolution - 1 && Y == yresolution - 1)
					{
						left = hm[Y][X - 1];
						down = hm[Y - 1][X];
					}
					else if (X == 0 && Y == yresolution - 1)
					{
						down = hm[Y - 1][X];
						right = hm[Y][X + 1];
					}
					else if (X == xresolution - 1 && Y == 0)
					{
						left = hm[Y][X - 1];
						up = hm[Y + 1][X];
					}
					else if (Y == 0)
					{
						left = hm[Y][X - 1];
						right = hm[Y][X + 1];
						up = hm[Y + 1][X];
					}
					else if (Y == yresolution - 1)
					{
						left = hm[Y][X - 1];
						right = hm[Y][X + 1];
						down = hm[Y - 1][X];
					}
					else if (X == 0)
					{
						right = hm[Y][X + 1];
						up = hm[Y + 1][X];
						down = hm[Y - 1][X];
					}
					else if (X == xresolution - 1)
					{
						left = hm[Y][X - 1];
						up = hm[Y + 1][X];
						down = hm[Y - 1][X];
					}
					else
					{
						left = hm[Y][X - 1];
						right = hm[Y][X + 1];
						up = hm[Y + 1][X];
						down = hm[Y - 1][X];
					}

					enum MIN_INDEX
					{
						CENTER = -1, LEFT, RIGHT, UP, DOWN
					};
					
					F32 minHeight = val;
					I32 minIndex = CENTER;

					if (left < minHeight)
					{
						minHeight = left;
						minIndex = LEFT;
					}
					if (right < minHeight)
					{
						minHeight = right;
						minIndex = RIGHT;
					}
					if (up < minHeight)
					{
						minHeight = up;
						minIndex = UP;
					}
					if (down < minHeight)
					{
						minHeight = down;
						minIndex = DOWN;
					}
					if (minHeight < val) 
					{
						F32 slope = std::min(minSlope, (val - minHeight));
						F32 valueToSteal = desc.deposition_speed * slope;

						if (carryingAmount > desc.carrying_capacity)
						{
							carryingAmount -= valueToSteal;
							hm[Y][X] += valueToSteal;
						}
						else 
						{
							if (carryingAmount + valueToSteal > desc.carrying_capacity)
							{
								F32 delta = carryingAmount + valueToSteal - desc.carrying_capacity;
								carryingAmount += delta;
								hm[Y][X] -= delta;
							}
							else
							{
								carryingAmount += valueToSteal;
								hm[Y][X] -= valueToSteal;
							}
						}

						if (minIndex == LEFT) X -= 1;
						else if (minIndex == RIGHT) X += 1;
						else if (minIndex == UP) Y += 1;
						else if (minIndex == DOWN) Y -= 1;

						if (X > xresolution - 1) X = xresolution - 1;
						if (Y > yresolution - 1) Y = yresolution - 1;
						if (Y < 0) Y = 0;
						if (X < 0) X = 0;
					}

				}

			}
		}
	}

}