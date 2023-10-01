#include <Common.hlsli>

struct VSInput
{
    float3 Pos : POSITION;
    float2 Uvs : TEX;
};

struct VSToPS
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEX;
};

VSToPS TextureVS(VSInput input)
{
    VSToPS output = (VSToPS)0;
    output.Position = mul(mul(float4(input.Pos, 1.0), objectData.model), frameData.viewprojection);
    output.TexCoord = input.Uvs;
    return output;
}

Texture2D<float4> Texture : register(t0);

float4 TexturePS(VSToPS input) : SV_TARGET
{
    float4 texColor = Texture.Sample(LinearWrapSampler, input.TexCoord) * float4(materialData.diffuse, 1.0) * materialData.albedoFactor;
    return texColor;
}