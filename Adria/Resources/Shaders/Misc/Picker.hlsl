#include <Common.hlsli>

struct PickingData
{
    float4 position;
    float4 normal;
};

Texture2D<float>  DepthTx   : register(t0);
Texture2D<float4> NormalTx  : register(t1);
RWStructuredBuffer<PickingData> PickingBuffer : register(u0);

[numthreads(1, 1, 1)]
void Picker()
{
    if (any(frameData.mouseNormalizedCoords > 1.0f) || any(frameData.mouseNormalizedCoords < 0.0f)) return;
    uint2 mouseCoords = uint2(frameData.mouseNormalizedCoords * frameData.screenResolution);

    float zw = DepthTx[mouseCoords].xx;
    float2 uv = (mouseCoords + 0.5f) / frameData.screenResolution;
    uv = uv * 2.0f - 1.0f;
    uv.y *= -1.0f;

    float4 worldSpacePosition = mul(float4(uv, zw, 1.0f), frameData.inverseViewProjection);
    float3 viewSpaceNormal = NormalTx[mouseCoords].xyz;
    viewSpaceNormal = 2.0 * viewSpaceNormal - 1.0;

    PickingData picking_data;
    picking_data.position = worldSpacePosition / worldSpacePosition.w;
    picking_data.normal = float4(normalize(mul(viewSpaceNormal, (float3x3) transpose(frameData.view))), 0.0f);
    PickingBuffer[0] = picking_data;
}