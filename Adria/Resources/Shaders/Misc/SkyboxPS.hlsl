#include "../Globals/GlobalsPS.hlsli"

TextureCube gCubeMap : register(t0);


struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

float4 main(VertexOut pin) : SV_Target
{
    
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
    return gCubeMap.Sample(linear_wrap_sampler, pin.PosL);
}