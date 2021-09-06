#include "../Globals/GlobalsPS.hlsli"

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

float4 main(VertexOut pin) : SV_Target
{
    return 0.0f;
}