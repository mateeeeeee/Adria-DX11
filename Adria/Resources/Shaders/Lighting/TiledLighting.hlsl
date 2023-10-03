#include <Common.hlsli>
#include <Util/LightUtil.hlsli>

#define MAX_TILE_LIGHTS 256
#define TILED_GROUP_SIZE 16

Texture2D NormalMetallicTx : register(t0);
Texture2D DiffuseRoughnessTx : register(t1);
Texture2D<float> DepthTx : register(t2);
StructuredBuffer<PackedLightData> LightsBuffer : register(t3);

RWTexture2D<float4> OutputTx : register(u0);
RWTexture2D<float4> DebugTx  : register(u1);

groupshared uint MinZ;
groupshared uint MaxZ;
groupshared uint TileLightIndices[MAX_TILE_LIGHTS];
groupshared uint TileNumLights;

[numthreads(TILED_GROUP_SIZE, TILED_GROUP_SIZE, 1)]
void TiledLightingCS(uint3 groupId : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID,
          uint3 groupThreadId : SV_GroupThreadID,
          uint  groupIndex : SV_GroupIndex)
{

    uint totalLights, unused;
    LightsBuffer.GetDimensions(totalLights, unused);

    float pixelMinZ = frameData.cameraFar;
    float pixelMaxZ = frameData.cameraNear;
    
    float2 tex = dispatchThreadId.xy / frameData.screenResolution;

    float depth = DepthTx.Load(int3(dispatchThreadId.xy, 0));
    float viewSpaceDepth = ConvertZToLinearDepth(depth);
   
    bool validPixel = viewSpaceDepth >= frameData.cameraNear && viewSpaceDepth < frameData.cameraFar;
    
    [flatten]
    if (validPixel)
    {
        pixelMinZ = min(pixelMinZ, viewSpaceDepth);
        pixelMaxZ = max(pixelMaxZ, viewSpaceDepth);
    }

    if (groupIndex == 0)
    {
        TileNumLights = 0;
        MinZ = 0x7F7FFFFF; 
        MaxZ = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (pixelMaxZ >= pixelMinZ)
    {
        InterlockedMin(MinZ, asuint(pixelMinZ));
        InterlockedMax(MaxZ, asuint(pixelMaxZ));
    }

    GroupMemoryBarrierWithGroupSync();

    float tileMinZ = asfloat(MinZ);
    float tileMaxZ = asfloat(MaxZ);

    float2 tileScale = frameData.screenResolution * rcp(float(2 * TILED_GROUP_SIZE));
    float2 tileBias = tileScale - float2(groupId.xy);
    
    float4 c1 = float4(frameData.projection._11 * tileScale.x, 0.0f, tileBias.x, 0.0f);
    float4 c2 = float4(0.0f, -frameData.projection._22 * tileScale.y, tileBias.y, 0.0f);
    float4 c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);

    float4 frustumPlanes[6];
    frustumPlanes[0] = c4 - c1;
    frustumPlanes[1] = c4 + c1;
    frustumPlanes[2] = c4 - c2;
    frustumPlanes[3] = c4 + c2;
    frustumPlanes[4] = float4(0.0f, 0.0f, 1.0f, -tileMinZ);
    frustumPlanes[5] = float4(0.0f, 0.0f, -1.0f, tileMaxZ);
    
    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        frustumPlanes[i] *= rcp(length(frustumPlanes[i].xyz));
    }
    
    
    for (uint lightIndex = groupIndex; lightIndex < totalLights; lightIndex += TILED_GROUP_SIZE * TILED_GROUP_SIZE)
    {
        PackedLightData light = LightsBuffer[lightIndex];       
        if (!light.active || light.castsShadows) continue;
        
        bool inFrustum = true;
        if(light.type != DIRECTIONAL_LIGHT)
        {
            [unroll]
            for (uint i = 0; i < 6; ++i)
            {
                float d = dot(frustumPlanes[i], float4(light.position.xyz, 1.0f));
                inFrustum = inFrustum && (d >= -light.range);
            }
        }

        [branch]
        if (inFrustum)
        {
            uint listIndex;
            InterlockedAdd(TileNumLights, 1, listIndex);
            TileLightIndices[listIndex] = lightIndex;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    float3 viewSpacePosition = GetViewSpacePosition(tex, depth);
    float4 normalMetallic = NormalMetallicTx.Load(int3(dispatchThreadId.xy, 0));
    
    float3 normal = 2 * normalMetallic.rgb - 1.0;
    float metallic = normalMetallic.a;
    float4 albedoRoughness = DiffuseRoughnessTx.Load(int3(dispatchThreadId.xy, 0));
    
    float3 V = normalize(0.0f.xxx - viewSpacePosition);
    float roughness = albedoRoughness.a;
        
    float3 Lo = 0.0f;
    if (all(dispatchThreadId.xy < frameData.screenResolution))
    {
        for (int i = 0; i < TileNumLights; ++i)
        {
            PackedLightData packedLightData = LightsBuffer[TileLightIndices[i]];
            LightData light = ConvertFromPackedLightData(packedLightData);
            switch (light.type)
            {
                case DIRECTIONAL_LIGHT:
                    Lo += DoDirectionalLightPBR(light, viewSpacePosition, normal, V, albedoRoughness.rgb, metallic, roughness);
                    break;
                case POINT_LIGHT:
                    Lo += DoPointLightPBR(light, viewSpacePosition, normal, V, albedoRoughness.rgb, metallic, roughness);
                    break;
                case SPOT_LIGHT:
                    Lo += DoSpotLightPBR(light, viewSpacePosition, normal, V, albedoRoughness.rgb, metallic, roughness);
                    break;
            }
        }
    }

    float4 shadingColor = OutputTx.Load(int3(int2(dispatchThreadId.xy), 0)) + float4(Lo, 1.0f);
    OutputTx[dispatchThreadId.xy] = shadingColor;
    
    
    if(computeData.visualizeTiled)
    {
        const float3 HeatMap[] =
        {
            float3(0, 0, 0),
		    float3(0, 0, 1),
		    float3(0, 1, 1),
		    float3(0, 1, 0),
		    float3(1, 1, 0),
		    float3(1, 0, 0),
        };
        const uint HeapMapMax = 5;
        float l = saturate((float) TileNumLights / computeData.lightsCountWhite) * HeapMapMax;
        float3 a = HeatMap[floor(l)];
        float3 b = HeatMap[ceil(l)];
    
        float4 debugColor = float4(lerp(a, b, l - floor(l)), 0.5f);
        DebugTx[dispatchThreadId.xy] = debugColor;
    }

}