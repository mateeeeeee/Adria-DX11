#include <Common.hlsli>
#include <Constants.hlsli>

static const float HBAO_BIAS = 0.1f;
static const int HBAO_NUM_STEPS = 4;
static const int HBAO_NUM_DIRECTIONS = 8; 

Texture2D NormalTx          : register(t1);
Texture2D<float> DepthTx    : register(t2);
Texture2D NoiseTx           : register(t3);

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

float Falloff(float distanceSquare)
{
    return distanceSquare * (-1.0f / postprocessData.hbaoR2) + 1.0;
}

float ComputeAO(float3 P, float3 N, float3 S)
{
    float3 V = S - P;
    float VdotV = dot(V, V);
    float NdotV = dot(N, V) * 1.0 / sqrt(VdotV);

   return clamp(NdotV - HBAO_BIAS, 0, 1) * clamp(Falloff(VdotV), 0, 1);
}

float2 RotateDirection(float2 dir, float2 cosSin)
{
    return float2(dir.x * cosSin.x - dir.y * cosSin.y,
              dir.x * cosSin.y + dir.y * cosSin.x);
}

float ComputeCoarseAO(float2 UV, float radiusInPixels, float3 rand, float3 viewSpacePosition, float3 viewSpaceNormal)
{
    float stepSizeInPixels = radiusInPixels / (HBAO_NUM_STEPS + 1);

    const float theta = M_PI_2 / HBAO_NUM_DIRECTIONS;
    float AO = 0;

    for (float directionIndex = 0; directionIndex < HBAO_NUM_DIRECTIONS; ++directionIndex)
    {
        float angle = theta * directionIndex;
        float2 direction = RotateDirection(float2(cos(angle), sin(angle)), rand.xy);
        float rayT = (rand.z * stepSizeInPixels + 1.0);

        for (float stepIndex = 0; stepIndex < HBAO_NUM_STEPS; ++stepIndex)
        {
            float2 SnappedUV = round(rayT * direction) / frameData.screenResolution + UV;
           
            float depth = DepthTx.Sample(LinearBorderSampler, SnappedUV);
            float3 S = GetViewSpacePosition(SnappedUV, depth);
            rayT += stepSizeInPixels;
            AO += ComputeAO(viewSpacePosition, viewSpaceNormal, S);
        }
    }

    AO *= (1.0f / (1.0f - HBAO_BIAS)) / (HBAO_NUM_DIRECTIONS * HBAO_NUM_STEPS);
    return clamp(1.0 - AO * 2.0, 0, 1);
}

float HBAO(VSToPS input) : SV_TARGET
{
    float depth = DepthTx.Sample(LinearBorderSampler, input.Tex);
    
    float3 viewSpacePosition = GetViewSpacePosition(input.Tex, depth);

    float3 viewSpaceNormal = NormalTx.Sample(LinearBorderSampler, input.Tex).rgb;
    viewSpaceNormal = 2 * viewSpaceNormal - 1.0;
    viewSpaceNormal = normalize(viewSpaceNormal);
   
    float radiusInPixels = postprocessData.hbaoRadiusToScreen / viewSpacePosition.z;
    
    float3 rand = NoiseTx.Sample(PointWrapSampler, input.Tex * postprocessData.ssaoNoiseScale).xyz; 
    float AO = ComputeCoarseAO(input.Tex, radiusInPixels, rand, viewSpacePosition, viewSpaceNormal);
    
    return pow(AO, postprocessData.hbaoPower);

}