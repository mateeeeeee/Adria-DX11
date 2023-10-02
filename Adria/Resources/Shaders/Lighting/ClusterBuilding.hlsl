#include <Common.hlsli>
#include <Util/LightUtil.hlsli>

#define CLUSTER_BUILDING_DISPATCH_SIZE_X 16
#define CLUSTER_BUILDING_DISPATCH_SIZE_Y 16
#define CLUSTER_BUILDING_DISPATCH_SIZE_Z 16

struct ClusterAABB
{
    float4 MinPoint;
    float4 MaxPoint;
};

RWStructuredBuffer<ClusterAABB> ClustersBuffer : register(u0);

float3 IntersectionZPlane(float3 B, float zDistance)
{
    float3 normal = float3(0.0, 0.0, -1.0);
    float3 d = B;
    float t = zDistance / d.z; 
    float3 result = t * d;
    return result;
}

[numthreads(1, 1, 1)]
void ClusterBuildingCS(uint3 groupId : SV_GroupID,
          uint3 dispatchThreadId     : SV_DispatchThreadID,
          uint3 groupThreadId        : SV_GroupThreadID,
          uint  groupIndex           : SV_GroupIndex)
{
    uint clusterSize, unused;
    ClustersBuffer.GetDimensions(clusterSize, unused);
    
    uint tileIndex = groupId.x +
                      groupId.y * CLUSTER_BUILDING_DISPATCH_SIZE_X +
                      groupId.z * (CLUSTER_BUILDING_DISPATCH_SIZE_X * CLUSTER_BUILDING_DISPATCH_SIZE_Y);
    float2 tileSize = rcp(float2(CLUSTER_BUILDING_DISPATCH_SIZE_X, CLUSTER_BUILDING_DISPATCH_SIZE_Y)); //screen_resolution;

    float3 maxPoint = GetViewSpacePosition((groupId.xy + 1) * tileSize, 1.0f);
    float3 minPoint = GetViewSpacePosition(groupId.xy * tileSize, 1.0f);

    float clusterNear = frameData.cameraNear * pow(frameData.cameraFar / frameData.cameraNear, groupId.z / float(CLUSTER_BUILDING_DISPATCH_SIZE_Z));
    float clusterFar  = frameData.cameraNear * pow(frameData.cameraFar / frameData.cameraNear, (groupId.z + 1) / float(CLUSTER_BUILDING_DISPATCH_SIZE_Z));

    float3 minPointNear = IntersectionZPlane(minPoint, clusterNear);
    float3 minPointFar  = IntersectionZPlane(minPoint, clusterFar);
    float3 maxPointNear = IntersectionZPlane(maxPoint, clusterNear);
    float3 maxPointFar  = IntersectionZPlane(maxPoint, clusterFar);

    float3 minPointAABB = min(min(minPointNear, minPointFar), min(maxPointNear, maxPointFar));
    float3 maxPointAABB = max(max(minPointNear, minPointFar), max(maxPointNear, maxPointFar));

    ClustersBuffer[tileIndex].MinPoint = float4(minPointAABB, 0.0);
    ClustersBuffer[tileIndex].MaxPoint = float4(maxPointAABB, 0.0);
}