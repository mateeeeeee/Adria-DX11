#include "../Globals/GlobalsCS.hlsli"

struct PickingData
{
    float4 position;
    float4 normal;
};

Texture2D<float> depthTx   : register(t0);
Texture2D<float4> NormalTx : register(t1);

RWStructuredBuffer<PickingData> PickingBuffer : register(u0);

[numthreads(1, 1, 1)]
void main( )
{
    if (any(mouse_normalized_coords > 1.0f) || any(mouse_normalized_coords < 0.0f)) return;

    uint2 mouse_coords = uint2(mouse_normalized_coords * screen_resolution);

    float zw = depthTx[mouse_coords].xx;
    float2 uv = (mouse_coords + 0.5f) / screen_resolution;
    uv = uv * 2.0f - 1.0f;
    uv.y *= -1.0f;
    float4 positionWS = mul(float4(uv, zw, 1.0f), inverse_view_projection);
    float3 normalVS = NormalTx[mouse_coords].xyz;
    normalVS = 2.0 * normalVS - 1.0;

    PickingData picking_data;
    picking_data.position = positionWS / positionWS.w;
    picking_data.normal = float4(normalize(mul(normalVS, (float3x3) transpose(view))), 0.0f);
    PickingBuffer[0] = picking_data;
}