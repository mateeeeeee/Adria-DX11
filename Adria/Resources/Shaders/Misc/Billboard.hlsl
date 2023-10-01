#include <Common.hlsli>

struct VSInput
{
    float3 Pos : POSITION;
    float2 Uvs : TEX;
};

struct VSToPS
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEX;
};

VSToPS BillboardVS(VSInput input)
{
    VSToPS output = (VSToPS)0;

    matrix modelMatrix = objectData.model;
    matrix modelView = mul(modelMatrix, frameData.view);

    modelView[0][0] = 1;
    modelView[0][1] = 0;
    modelView[0][2] = 0;
    modelView[1][0] = 0;
    modelView[1][1] = 1;
    modelView[1][2] = 0;
    modelView[2][0] = 0;
    modelView[2][1] = 0;
    modelView[2][2] = 1;

    float4 viewPosition = mul(float4(input.Pos, 1.0), modelView);
    output.Position = mul(viewPosition, frameData.projection);
    output.TexCoord = input.Uvs;
    return output;
}