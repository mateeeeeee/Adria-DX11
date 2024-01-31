#include <Common.hlsli>
#include <Util/ToneMapUtil.hlsli>

Texture2D HDRTx : register(t0);
#if TONY_MCMAPFACE
Texture3D<float3> LUT : register(t1);
#endif

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

float4 ToneMap(VSToPS input) : SV_TARGET
{
    float4 color = HDRTx.Sample(LinearWrapSampler, input.Tex);
#if REINHARD
    float4 toneMappedColor = float4(ReinhardToneMapping(color.xyz * postprocessData.toneMapExposure), 1.0);
#elif LINEAR
    float4 toneMappedColor = float4(LinearToneMapping(color.xyz * postprocessData.toneMapExposure), 1.0);
#elif HABLE 
    float4 toneMappedColor = float4(HableToneMapping(color.xyz * postprocessData.toneMapExposure), 1.0);
#elif TONY_MCMAPFACE
    float4 toneMappedColor = float4(TonyMcMapface(LUT, LinearClampSampler, color.xyz * postprocessData.toneMapExposure), 1.0);
#else 
    float4 toneMappedColor = float4(color.xyz * postprocessData.toneMapExposure, 1.0f);
#endif
    return toneMappedColor;
}