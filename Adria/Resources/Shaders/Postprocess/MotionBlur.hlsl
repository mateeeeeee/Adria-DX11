#include <Common.hlsli>

Texture2D<float4> SceneTx : register(t0);
Texture2D<float2> MotionVectorsTx : register(t1);


struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

static const int SAMPLE_COUNT = 16;

float4 MotionBlur(VSToPS input) : SV_TARGET
{
    float2 uv = input.Tex;
    float2 velocity = MotionVectorsTx.SampleLevel(LinearWrapSampler, input.Tex, 0) / SAMPLE_COUNT;

    float4 color = SceneTx.Sample(LinearWrapSampler, uv);
    uv += velocity;
    for (int i = 1; i < SAMPLE_COUNT; ++i, uv += velocity)
    {
        float4 currentColor = SceneTx.Sample(LinearClampSampler, uv);
        color += currentColor;
    }
    float4 finalColor = color / SAMPLE_COUNT;
    return finalColor;
}