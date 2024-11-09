#pragma once
#include <array>

namespace adria
{

	enum ESkyParams : Uint16
	{
		ESkyParam_A = 0,
		ESkyParam_B,
		ESkyParam_C,
		ESkyParam_D,
		ESkyParam_E,
		ESkyParam_F,
		ESkyParam_G,
		ESkyParam_I,
		ESkyParam_H,
		ESkyParam_Z,
		ESkyParam_Count
	};

	using SkyParameters = std::array<Vector3, ESkyParam_Count>;

	SkyParameters CalculateSkyParameters(Float turbidity, Float albedo, Vector3 const& sun_direction);
}