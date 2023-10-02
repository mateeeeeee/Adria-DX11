#include <Common.hlsli>

#define COMPUTE_WORK_GROUP_DIM 32

static const float PI = 3.14159265359;
static const float g = 9.81;
static const float KM = 370.0;


Texture2D<float4>   PhasesTx      : register(t0);
RWTexture2D<float4> DeltaPhasesTx : register(u0);

float Omega(float k)
{
    return sqrt(g * k * (1.0 + k * k / KM * KM));
}
float Mod(float x, float y)
{
    return x - y * floor(x / y);
}

[numthreads(COMPUTE_WORK_GROUP_DIM, COMPUTE_WORK_GROUP_DIM, 1)]
void PhaseCS(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchID.xy;
    int resolution = computeData.resolution;
    
    float n = (pixelCoord.x < 0.5f * uint(resolution)) ? pixelCoord.x : pixelCoord.x - float(resolution);
    float m = (pixelCoord.y < 0.5f * uint(resolution)) ? pixelCoord.y : pixelCoord.y - float(resolution);

    float2 waveVector = (2.f * PI * float2(n, m)) / computeData.oceanSize;
    float k = length(waveVector);
    float slowdownFactor = 1.f;

    float deltaPhase = Omega(k) * computeData.deltaTime * slowdownFactor;
    float phase = PhasesTx.Load(uint3(pixelCoord, 0)).r;
    phase = Mod(phase + deltaPhase, 2.f * PI);
    DeltaPhasesTx[pixelCoord] = float4(phase, 0.f, 0.f, 0.f);
}