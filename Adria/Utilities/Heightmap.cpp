#include "Heightmap.h"
#include "Cpp/FastNoiseLite.h"
#include "Image.h"
#include "Random.h"

namespace adria
{
	
	static constexpr FastNoiseLite::NoiseType GetNoiseType(NoiseType type)
	{
		switch (type)
		{
		case NoiseType::OpenSimplex2:
			return FastNoiseLite::NoiseType_OpenSimplex2;
		case NoiseType::OpenSimplex2S:
			return FastNoiseLite::NoiseType_OpenSimplex2S;
		case NoiseType::Cellular:
			return FastNoiseLite::NoiseType_OpenSimplex2;
		case NoiseType::ValueCubic:
			return FastNoiseLite::NoiseType_ValueCubic;
		case NoiseType::Value:
			return FastNoiseLite::NoiseType_Value;
		case NoiseType::Perlin:
		default:
			return FastNoiseLite::NoiseType_Perlin;
		}

		return FastNoiseLite::NoiseType_Perlin;
	}
	static constexpr FastNoiseLite::FractalType GetFractalType(FractalType type)
	{
		switch (type)
		{
		case FractalType::FBM:
			return FastNoiseLite::FractalType_FBm;
		case FractalType::Ridged:
			return FastNoiseLite::FractalType_Ridged;
		case FractalType::PingPong:
			return FastNoiseLite::FractalType_PingPong;
		case FractalType::None:
		default:
			return FastNoiseLite::FractalType_None;
		}

		return FastNoiseLite::FractalType_None;
	}
	static float DepositSediment(float c, float max_diff, float talus, float distance, float total_diff)
	{
		return (distance > talus) ? (c * (max_diff - talus) * (distance / total_diff)) : 0.0f;
	}

	Heightmap::Heightmap(NoiseDesc const& desc)
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

		float max_height_achieved = std::numeric_limits<float>::min();
		float min_height_achieved = std::numeric_limits<float>::max();

		for (uint32 z = 0; z < desc.depth; z++)
		{
			hm[z].resize(desc.width);
			for (uint32 x = 0; x < desc.width; x++)
			{
				float xf = x * desc.noise_scale / desc.width;
				float zf = z * desc.noise_scale / desc.depth;

				float height = noise.GetNoise(xf, zf) * desc.max_height;
				if (height > max_height_achieved) max_height_achieved = height;
				if (height < min_height_achieved) min_height_achieved = height;
				hm[z][x] = height;
			}
		}

		auto scale = [=](float h) -> float
		{
			return (h - min_height_achieved) / (max_height_achieved - min_height_achieved)
				* 2 * desc.max_height - desc.max_height;
		};

		for (uint32 z = 0; z < desc.depth; z++)
		{
			for (uint32 x = 0; x < desc.width; x++)
			{
				hm[z][x] = scale(hm[z][x]);
			}
		}
	}
	Heightmap::Heightmap(std::string_view heightmap_path, uint32 max_height)
	{
		Image img(heightmap_path, 1);

		uint8 const* image_data = img.Data<uint8>();

		hm.resize(img.Height());
		for (uint64 z = 0; z < img.Height(); ++z)
		{
			hm[z].resize(img.Width());
			for (uint64 x = 0; x < img.Width(); ++x)
			{
				hm[z][x] = image_data[z * img.Width() + x] / 255.0f * max_height;
			}
		}
	}
	float Heightmap::HeightAt(uint64 x, uint64 z) const
	{
		return hm[z][x];
	}
	uint64 Heightmap::Width() const
	{
		return hm[0].size();
	}
	uint64 Heightmap::Depth() const
	{
		return hm.size();
	}

	void Heightmap::ApplyThermalErosion(ThermalErosionDesc const& desc)
	{
		float v1, v2, v3, v4, v5, v6, v7, v8, v9;	
		float d1, d2, d3, d4, d5, d6, d7, d8;		
		float talus = desc.talus;//4.0f / resolution
		float c = desc.c; //0.5f;					

		for (size_t k = 0; k < desc.iterations; k++)
		{
			for (size_t j = 0; j < hm.size(); j++)
			{
				for (size_t i = 0; i < hm[j].size(); i++)
				{

					float max_diff = 0.0f;
					float total_diff = 0.0f;

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

					float d_array[] = { d1, d2, d3, d4, d5, d6, d7, d8 };
					for (float d : d_array)
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
	void Heightmap::ApplyHydraulicErosion(HydraulicErosionDesc const& desc)
	{
		uint64 drops = (uint64)desc.drops;

		uint64 xresolution = Width();
		uint64 yresolution = Depth();

		IntRandomGenerator<uint64> x_random(0, xresolution - 1);
		IntRandomGenerator<uint64> y_random(0, yresolution - 1);

		for (uint64 drop = 0; drop < drops; drop++)
		{
			// Get random coordinates to drop the water droplet at
			uint64 X = x_random();
			uint64 Y = y_random();

			float carryingAmount = 0.0f;
			float minSlope = 1.15f;

			if (hm[Y][X] > 0.0f)
			{
				for (int32 iter = 0; iter < desc.iterations; iter++)
				{
					float val = hm[Y][X];
					float left = 1000.0f;
					float right = 1000.0f;
					float up = 1000.0f;
					float down = 1000.0f;

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
					
					float minHeight = val;
					int32 minIndex = CENTER;

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
						float slope = std::min(minSlope, (val - minHeight));
						float valueToSteal = desc.deposition_speed * slope;

						if (carryingAmount > desc.carrying_capacity)
						{
							carryingAmount -= valueToSteal;
							hm[Y][X] += valueToSteal;
						}
						else 
						{
							if (carryingAmount + valueToSteal > desc.carrying_capacity)
							{
								float delta = carryingAmount + valueToSteal - desc.carrying_capacity;
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