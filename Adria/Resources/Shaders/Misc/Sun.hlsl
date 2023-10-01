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


VSToPS SunVS(VSInput input)
{
    VSToPS output = (VSToPS)0;

    float4x4 modelMatrix = objectData.model;
    modelMatrix[3][0] += frameData.inverseView[3][0];
    modelMatrix[3][1] += frameData.inverseView[3][1];
    modelMatrix[3][2] += frameData.inverseView[3][2];

    float4x4 modelView = mul(modelMatrix, frameData.view);
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
    float4 clipSpacePosition = mul(viewPosition, frameData.projection); 
    output.Position = float4(clipSpacePosition.xy, clipSpacePosition.w - 0.001f, clipSpacePosition.w); 
    output.TexCoord = input.Uvs;
    return output;
}