#include <Common.hlsli>

struct VSInput
{
    float3 Pos : POSITION;
};

struct VSToPS
{
    float4 Position : SV_POSITION;
};

VSToPS SolidVS(VSInput input)
{
    VSToPS output = (VSToPS)0;
    output.Position = mul(mul(float4(input.Pos, 1.0), objectData.model), frameData.viewprojection);
    return output;
}

float4 SolidPS(VSToPS input) : SV_Target
{
    return float4(materialData.diffuse, 1.0);
}