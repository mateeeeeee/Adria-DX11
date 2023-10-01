#include <Common.hlsli>

Texture2D<float4> Texture : register(t0);


struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};


float4 CopyTexture(VSToPS input) : SV_Target0
{
    return Texture.Sample(LinearWrapSampler, input.Tex);
}