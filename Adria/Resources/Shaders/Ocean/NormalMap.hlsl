#include <Common.hlsli>

#define WORK_GROUP_DIM 32

static const float LAMBDA = 1.2f;

Texture2D<float4> DisplacementTx : register(t0);
RWTexture2D<float4> NormalTx : register(u0);

[numthreads(WORK_GROUP_DIM, WORK_GROUP_DIM, 1)]
void NormalMapCS(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadId.xy;
    int resolution = computeData.resolution;
    float texel = 1.f / resolution;
    float texelSize = computeData.oceanSize * texel;

    float3 tmpRight    = DisplacementTx.Load(uint3(uint2(clamp(pixelCoord.x + 1, 0, resolution - 1), pixelCoord.y), 0)).xyz;
    float3 tmpLeft     = DisplacementTx.Load(uint3(uint2(clamp(pixelCoord.x - 1, 0, resolution - 1), pixelCoord.y), 0)).xyz;
    float3 tmpTop      = DisplacementTx.Load(uint3(uint2(pixelCoord.x, clamp(pixelCoord.y - 1, 0, resolution - 1)), 0)).xyz;
    float3 tmpBottom   = DisplacementTx.Load(uint3(uint2(pixelCoord.x, clamp(pixelCoord.y + 1, 0, resolution - 1)), 0)).xyz;

    float3 center   = DisplacementTx.Load(uint3(pixelCoord, 0)).xyz;
    float3 right    = float3( texelSize, 0.f, 0.f) + tmpRight;
    float3 left     = float3(-texelSize, 0.f, 0.f) + tmpLeft;
    float3 top      = float3(0.f, 0.f, -texelSize) + tmpTop;
    float3 bottom   = float3(0.f, 0.f,  texelSize) + tmpBottom;
    
    float3 topRight    = cross(right - center, top - center);
    float3 topLeft     = cross(top - center, left - center);
    float3 bottomLeft  = cross(left - center, bottom - center);
    float3 bottomRight = cross(bottom - center, right - center);

    float2 dDx = (tmpRight.xz - tmpLeft.xz) * 0.5f; 
    float2 dDy = (tmpBottom.xz - tmpTop.xz) * 0.5f;

    float J = (1.0 + dDx.x * LAMBDA) * (1.0 + dDy.y * LAMBDA) - dDx.y * dDy.x * LAMBDA * LAMBDA;
    
    float foamFactor = max(-J, 0.0); 
    NormalTx[pixelCoord] = float4(normalize(topRight + bottomRight + topLeft + bottomLeft), foamFactor);
}

