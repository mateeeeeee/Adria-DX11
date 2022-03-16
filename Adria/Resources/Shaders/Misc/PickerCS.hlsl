#include "../Globals/GlobalsCS.hlsli"

struct PickingData
{
    float4 position;
    float4 normal;
};

Texture2D<float> depthTx   : register(t0);
Texture2D<float4> normalTx : register(t1);

RWStructuredBuffer<PickingData> PickingBuffer : register(u0);

[numthreads(1, 1, 1)]
void main( )
{
    float zw = depthTx[mouse_position].xx;
    float2 uv = (mouse_position + 0.5f) / screen_resolution;
    uv = uv * 2.0f - 1.0f;
    uv.y *= -1.0f;
    float4 positionWS = mul(float4(uv, zw, 1.0f), inverse_view_projection);
    float3 normalVS = normalTx[mouse_position].xyz;

    PickingData picking_data;
    picking_data.position = positionWS / positionWS.w;
    picking_data.normal = mul(float4(normalVS, 0.0f), transpose(view));
    PickingBuffer[0] = picking_data;
}