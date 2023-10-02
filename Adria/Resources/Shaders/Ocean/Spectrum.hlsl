#include <Common.hlsli>

#define COMPUTE_WORK_GROUP_DIM 32

static const float PI = 3.14159265359f;
static const float g = 9.81f;
static const float KM = 370.f;


Texture2D<float4>   PhasesTx            : register(t0);
Texture2D<float4>   InitialSpectrumTx   : register(t1);
RWTexture2D<float4> SpectrumTx        : register(u0);

float2 MultiplyComplex(float2 a, float2 b)
{
    return float2(a.x * b.x - a.y * b.y, a.y * b.x + a.x * b.y);
}
float2 MultiplyByI(float2 z)
{
    return float2(-z.y, z.x);
}
float Omega(float k)
{
    return sqrt(g * k * (1.0 + k * k / KM * KM));
}

[numthreads(COMPUTE_WORK_GROUP_DIM, COMPUTE_WORK_GROUP_DIM, 1)]
void SpectrumCS(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchID.xy;
    int resolution = computeData.resolution;
    float n = (pixelCoord.x < 0.5f * uint(resolution)) ? pixelCoord.x : pixelCoord.x - float(resolution);
    float m = (pixelCoord.y < 0.5f * uint(resolution)) ? pixelCoord.y : pixelCoord.y - float(resolution);
    float2 waveVector = (2.f * PI * float2(n, m)) / computeData.oceanSize;

    float phase = PhasesTx.Load(uint3(pixelCoord, 0)).r;
    float2 phaseVector = float2(cos(phase), sin(phase));
    uint2 coords = (uint2(resolution, resolution) - pixelCoord) % (resolution - 1);

    float2 h0 = float2(InitialSpectrumTx.Load(uint3(pixelCoord, 0)).r, 0.f);
    float2 h0Star = float2(InitialSpectrumTx.Load(uint3(coords, 0)).r, 0.f);
    h0Star.y *= -1.f;

    float2 h = MultiplyComplex(h0, phaseVector) + MultiplyComplex(h0Star, float2(phaseVector.x, -phaseVector.y));
    float2 hX = -MultiplyByI(h * (waveVector.x / length(waveVector))) * computeData.oceanChoppiness;
    float2 hZ = -MultiplyByI(h * (waveVector.y / length(waveVector))) * computeData.oceanChoppiness;

    if (waveVector.x == 0.0 && waveVector.y == 0.0)
    {
        h  = float2(0.f, 0.f);
        hX = float2(0.f, 0.f);
        hZ = float2(0.f, 0.f);
    }
    SpectrumTx[pixelCoord] = float4(hX + MultiplyByI(h), hZ);
}