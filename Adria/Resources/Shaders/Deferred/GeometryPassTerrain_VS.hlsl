#include "../Globals/GlobalsVS.hlsli"


struct VS_INPUT
{
    float3 Position : POSITION;
    float2 Uvs      : TEX;
    float3 Normal   : NORMAL;
};



struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float4 PosWS    : POS;
    float2 Uvs      : TEX;
    float3 NormalVS : NORMAL0;
    float3 NormalWS : NORMAL1;
};


VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT Output;
    
    float4 pos = mul(float4(input.Position, 1.0), model);
    Output.PosWS = pos;
    Output.Position = mul(pos, viewprojection);

    Output.Uvs = input.Uvs;

	// Transform the normal to world space
    float3 normal_ws = mul(input.Normal, (float3x3) transposed_inverse_model);
    Output.NormalVS = mul(normal_ws, (float3x3) transpose(inverse_view));
    Output.NormalWS = normal_ws;

    return Output;
}