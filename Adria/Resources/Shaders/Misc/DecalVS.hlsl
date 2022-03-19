#include "../Globals/GlobalsVS.hlsli"

struct VS_INPUT
{
    float3 Pos : POSITION;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
};

cbuffer DecalCBuffer : register(b11)
{
    row_major matrix decal_projection;
    row_major matrix decal_viewprojection;
    row_major matrix decal_inverse_viewprojection;
    float2 aspect_ratio;
}



VS_OUTPUT main(VS_INPUT vin)
{
    VS_OUTPUT vout = (VS_OUTPUT) 0;
    float4 world_pos = mul(float4(vin.Pos, 1.0f), decal_inverse_viewprojection);
    vout.Position = mul(world_pos, viewprojection);
    return vout;
}