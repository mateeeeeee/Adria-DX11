#include <Common.hlsli>

Texture2D DiffuseTx : register(t0);

struct VSInput
{
    float3 Pos : POSITION;
#if TRANSPARENT
    float2 TexCoords : TEX;
#endif
};

struct VSToPS
{
    float4 Pos : SV_POSITION;
#if TRANSPARENT
    float2 TexCoords : TEX;
#endif
};


VSToPS ShadowVS(VSInput input)
{
    VSToPS output;
    float4 pos = float4(input.Pos, 1.0f);
    pos = mul(pos, objectData.model);
    pos = mul(pos, shadowData.lightViewProjection);
    output.Pos = pos;
    
#if TRANSPARENT
    output.TexCoords = input.TexCoords;
#endif
    return output;
}


void ShadowPS(VSToPS input)
{
#if TRANSPARENT
    if( DiffuseTx.Sample(LinearWrapSampler,input.TexCoords).a < 0.1 ) discard;
#endif
}