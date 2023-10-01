#include <Common.hlsli>
#include <Util/DitherUtil.hlsli>

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

Texture2D SunTx : register(t0);


float4 GodRays(VSToPS input) : SV_TARGET
{
    float2 texCoord = input.Tex;
    float3 color = SunTx.SampleLevel(LinearClampSampler, texCoord, 0).rgb;
    
    float2 lightPosition = lightData.screenSpacePosition.xy;
    float2 deltaTexCoord = (texCoord - lightPosition);
    const int NUM_SAMPLES = 32;
    deltaTexCoord *= lightData.godraysDensity / NUM_SAMPLES;
    
    float illuminationDecay = 1.0f;
    float3 accumulatedGodRays = float3(0.0f,0.0f,0.0f);
    float3 accumulated = 0.0f;
    
    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        texCoord.xy -= deltaTexCoord;
        float3 sample = SunTx.SampleLevel(LinearClampSampler, texCoord.xy, 0).rgb;
        sample *= illuminationDecay * lightData.godraysWeight;
        accumulated += sample;
        illuminationDecay *= lightData.godraysDecay;
    }
    
    accumulated *= lightData.godraysExposure;
    return float4(color + accumulated, 1.0f);
}
