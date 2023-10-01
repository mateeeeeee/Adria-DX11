#include <Common.hlsli>

struct VSToPS
{
    float4 Pos  : SV_POSITION;
    float3 PosL : POSITION;
};

float4 UniformSky(VSToPS input) : SV_Target
{
    return weatherData.skyColor;
}