#include <Common.hlsli>
#include <Util/LightUtil.hlsli>
#include <Util/DitherUtil.hlsli>
#include <Util/ShadowUtil.hlsli>

struct LightGrid
{
    uint Offset;
    uint LightCount;
};

Texture2D NormalMetallicTx      : register(t0);
Texture2D DiffuseRoughnessTx    : register(t1);
Texture2D<float> DepthTx        : register(t2);

StructuredBuffer<PackedLightData> LightsBuffer      : register(t3);
StructuredBuffer<uint> LightIndexList               : register(t4);    
StructuredBuffer<LightGrid> LightGridBuffer         : register(t5);   

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

float4 ClusterLightingPS(VSToPS input) : SV_TARGET
{
    float4 normalMetallic = NormalMetallicTx.Sample(LinearWrapSampler, input.Tex);
    float3 viewSpaceNormal = 2 * normalMetallic.rgb - 1.0;
    float  metallic = normalMetallic.a;

    float  depth = DepthTx.Sample(LinearWrapSampler, input.Tex);
    float3 viewSpacePosition = GetViewSpacePosition(input.Tex, depth);
    float3 V = normalize(0.0f.xxx - viewSpacePosition);
    float  linearDepth = ConvertZToLinearDepth(depth);

    float4 diffuseRoughness = DiffuseRoughnessTx.Sample(LinearWrapSampler, input.Tex);
    float roughness = diffuseRoughness.a;

    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    
    uint zCluster = uint(max((log2(linearDepth) - log2(frameData.cameraNear)) * 16.0f / log2(frameData.cameraFar / frameData.cameraNear), 0.0f));
    uint2 clusterDim = ceil(frameData.screenResolution / float2(16, 16));
    uint3 tiles = uint3(uint2(input.Pos.xy / clusterDim), zCluster);

    uint tileIndex = tiles.x + 16 * tiles.y + (256) * tiles.z;
    
    uint lightCount = LightGridBuffer[tileIndex].LightCount;
    uint lightOffset = LightGridBuffer[tileIndex].Offset;

    for (uint i = 0; i < lightCount; i++)
    {
        uint lightIndex = LightIndexList[lightOffset + i];
        PackedLightData packedLightData = LightsBuffer[lightIndex];
        LightData light = ConvertFromPackedLightData(packedLightData);

        switch (light.type)
        {
            case DIRECTIONAL_LIGHT:
                Lo += DoDirectionalLightPBR(light, viewSpacePosition, viewSpaceNormal, V, diffuseRoughness.rgb, metallic, roughness);
                break;
            case POINT_LIGHT:
                Lo += DoPointLightPBR(light, viewSpacePosition, viewSpaceNormal, V, diffuseRoughness.rgb, metallic, roughness);
                break;
            case SPOT_LIGHT:
                Lo += DoSpotLightPBR(light, viewSpacePosition, viewSpaceNormal, V, diffuseRoughness.rgb, metallic, roughness);
                break;
            default:
                return float4(1, 0, 0, 1);
        }
    }
    return float4(Lo, 1.0f);
}