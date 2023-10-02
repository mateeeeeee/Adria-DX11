#include <Common.hlsli>
#include <Util/LightUtil.hlsli>

#define GROUP_SIZE (16*16*4)
#define MAX_CLUSTER_LIGHTS 128
#define CLUSTER_CULLING_DISPATCH_SIZE_X 1
#define CLUSTER_CULLING_DISPATCH_SIZE_Y 1
#define CLUSTER_CULLING_DISPATCH_SIZE_Z 4


struct ClusterAABB
{
    float4 MinPoint;
    float4 MaxPoint;
};

struct LightGrid
{
    uint Offset;
    uint LightCount;
};


StructuredBuffer<ClusterAABB> ClustersBuffer      : register(t0);
StructuredBuffer<StructuredLight> LightsBuffer    : register(t1);

RWStructuredBuffer<uint> LightIndexCounter    : register(u0); 
RWStructuredBuffer<uint> LightIndexList       : register(u1); 
RWStructuredBuffer<LightGrid> LightGridBuffer : register(u2); 

groupshared StructuredLight SharedLights[GROUP_SIZE];

bool LightIntersectsCluster(StructuredLight light, ClusterAABB cluster)
{
    if (light.type == DIRECTIONAL_LIGHT) return true;
    float3 closest = max(cluster.MinPoint, min(light.position, cluster.MaxPoint)).xyz;
    float3 dist = closest - light.position.xyz;
    return dot(dist, dist) <= (light.range * light.range);
}

[numthreads(16, 16, 4)]
void ClusterCullingCS(uint3 groupId : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID,
          uint3 groupThreadId : SV_GroupThreadID,
          uint groupIndex : SV_GroupIndex)
{
    if (all(dispatchThreadId == 0))
    {
        LightIndexCounter[0] = 0;
    }
        
    uint visibleLightCount = 0;
    uint visibleLightIndices[MAX_CLUSTER_LIGHTS];
    
    uint clusterIndex = groupIndex + GROUP_SIZE * groupId.z;
    ClusterAABB cluster = ClustersBuffer[clusterIndex];
    
    uint lightOffset = 0;
    uint lightCount, unused;
    LightsBuffer.GetDimensions(lightCount, unused);

    while (lightOffset < lightCount)
    {
        uint batchSize = min(GROUP_SIZE, lightCount - lightOffset);
        if(groupIndex < batchSize)
        {
            uint lightIndex = lightOffset + groupIndex;
            StructuredLight light = LightsBuffer[lightIndex];
            SharedLights[groupIndex] = light;
        }
        
        GroupMemoryBarrierWithGroupSync();

        for (uint i = 0; i < batchSize; i++)
        {
            StructuredLight light = LightsBuffer[i];
            if (!light.active || light.castsShadows) continue;
            if (visibleLightCount < MAX_CLUSTER_LIGHTS && LightIntersectsCluster(light, cluster))
            {
                visibleLightIndices[visibleLightCount] = lightOffset + i;
                visibleLightCount++;
            }
        }
        lightOffset += batchSize;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    uint offset = 0;
    InterlockedAdd(LightIndexCounter[0], visibleLightCount, offset);

    for (uint i = 0; i < visibleLightCount; i++)
    {
        LightIndexList[offset + i] = visibleLightIndices[i];
    }
    
    LightGridBuffer[clusterIndex].Offset = offset; 
    LightGridBuffer[clusterIndex].LightCount = visibleLightCount;
}
