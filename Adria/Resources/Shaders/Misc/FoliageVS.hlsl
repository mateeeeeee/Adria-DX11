#include "../Globals/GlobalsVS.hlsli"

struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 Uvs : TEX;
    float3 Offset : INSTANCE_OFFSET;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEX;
};

VS_OUTPUT main(VS_INPUT vin)
{
    VS_OUTPUT vout;

    matrix model_matrix = model;
    model_matrix[3].xyz += vin.Offset;
    matrix model_view = mul(model_matrix, view);

    model_view[0][0] = 1;
    model_view[0][1] = 0;
    model_view[0][2] = 0;
    model_view[1][0] = 0;
    model_view[1][1] = 1;
    model_view[1][2] = 0;
    model_view[2][0] = 0;
    model_view[2][1] = 0;
    model_view[2][2] = 1;

    float4 ViewPosition = mul(float4(vin.Pos, 1.0), model_view);

    vout.Position = mul(ViewPosition, projection);
    vout.TexCoord = vin.Uvs;
    return vout;
}