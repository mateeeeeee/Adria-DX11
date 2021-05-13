#include "../CBuffers/PerObject.hlsli"
#include "../CBuffers/PerFrame.hlsli"

struct VS_INPUT
{
    float3 Pos : POSITION;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
};




VS_OUTPUT main(VS_INPUT vin)
{
    VS_OUTPUT vout;
    vout.Pos = mul(mul(float4(vin.Pos, 1.0), model), viewprojection);

    return vout;
}