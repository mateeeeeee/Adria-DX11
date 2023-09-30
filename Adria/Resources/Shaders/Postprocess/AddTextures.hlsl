#include <Common.hlsli>

Texture2D<float4> Texture1 : register(t0);
Texture2D<float4> Texture2 : register(t1);

struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

float4 AddTextures(PSInput input) : SV_Target0
{
    float4 color1 = Texture1.Sample(LinearWrapSampler, input.Tex);
    float4 color2 = Texture2.Sample(LinearWrapSampler, input.Tex);
    return color1 + color2;
}