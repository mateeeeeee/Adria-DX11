#include <Common.hlsli>

struct VSInput
{
    float3 Position : POSITION;
    float2 Uvs      : TEX;
    float3 Normal   : NORMAL;
};

struct VSToPS
{
    float4 Position : SV_POSITION;
    float4 PosWS    : POS;
    float2 Uvs      : TEX;
    float3 NormalVS : NORMAL0;
    float3 NormalWS : NORMAL1;
};


VSToPS TerrainVS(VSInput input)
{
    VSToPS Output = (VSToPS)0;
    
    float4 pos = mul(float4(input.Position, 1.0), objectData.model);
    Output.PosWS = pos;
    Output.Position = mul(pos, frameData.viewprojection);
    Output.Uvs = input.Uvs;

	float3 worldSpaceNormal = mul(input.Normal, (float3x3) objectData.transposedInverseModel);
    Output.NormalVS = mul(worldSpaceNormal, (float3x3) transpose(frameData.inverseView));
    Output.NormalWS = worldSpaceNormal;

    return Output;
}

cbuffer TerrainCBuffer : register(b9)
{
    float2 textureScale;
    int    oceanActive;
};

Texture2D GrassTx   : register(t0);
Texture2D BaseTx    : register(t1);
Texture2D RockTx    : register(t2);
Texture2D SandTx    : register(t3);
Texture2D LayerTx   : register(t4);

struct PSOutput
{
    float4 NormalMetallic : SV_TARGET0;
    float4 DiffuseRoughness : SV_TARGET1;
    float4 EmissiveAO : SV_TARGET2;
};


PSOutput PackGBuffer(float3 baseColor, float3 viewSpaceNormal, float3 emissive, float roughness, float metallic, float ao)
{
    PSOutput Out = (PSOutput)0;
    Out.NormalMetallic = float4(0.5 * viewSpaceNormal + 0.5, metallic);
    Out.DiffuseRoughness = float4(baseColor, roughness);
    Out.EmissiveAO = float4(emissive, ao);
    return Out;
}



PSOutput TerrainPS(VSToPS input)
{

    input.Uvs.y = 1 - input.Uvs.y;
    float3 normal = normalize(input.NormalWS);
    
    float4 color = BaseTx.Sample(LinearWrapSampler, input.Uvs);
    
    float4 grass = GrassTx.Sample(LinearWrapSampler, input.Uvs);
    float4 rock  = RockTx.Sample(LinearWrapSampler, input.Uvs);
    float4 sand  = SandTx.Sample(LinearWrapSampler, input.Uvs);
    float4 layer = LayerTx.Sample(LinearWrapSampler, input.Uvs / textureScale); 
    
    color = lerp(color, rock, layer.r);
    color = lerp(color, sand,  layer.g);
    color = lerp(color, grass, layer.b);
    color *= 0.7f;
    
    if (oceanActive) color.rgb *= 0.5 + 0.5 * max(0, min(1, input.PosWS.y * 0.5 + 0.5));
    return PackGBuffer(color.xyz, normalize(input.NormalVS), float3(0, 0, 0),
    1.0f, 0.0f, 1.0f); 
}