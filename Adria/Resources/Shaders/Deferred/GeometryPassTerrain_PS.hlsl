#include "../Globals/GlobalsPS.hlsli"


Texture2D txGrass   : register(t0);
Texture2D txBase    : register(t1);
Texture2D txRock    : register(t2);
Texture2D txSand    : register(t3);
Texture2D txLayer   : register(t4);

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float4 PosWS    : POS;
    float2 Uvs      : TEX;
    float3 NormalVS : NORMAL0;
    float3 NormalWS : NORMAL1;
};


struct PS_GBUFFER_OUT
{
    float4 NormalMetallic : SV_TARGET0;
    float4 DiffuseRoughness : SV_TARGET1;
    float4 EmissiveAO : SV_TARGET2;
};


PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float3 NormalVS, float3 emissive, float roughness, float metallic, float ao)
{
    PS_GBUFFER_OUT Out;

    Out.NormalMetallic = float4(0.5 * NormalVS + 0.5, metallic);
    Out.DiffuseRoughness = float4(BaseColor, roughness);
    Out.EmissiveAO = float4(emissive, ao);
    return Out;
}



PS_GBUFFER_OUT main(VS_OUTPUT In)
{

    In.Uvs.y = 1 - In.Uvs.y;
    float3 normal = normalize(In.NormalWS);
    
    float4 color = txBase.Sample(linear_wrap_sampler, In.Uvs);
    
    float4 grass = txGrass.Sample(linear_wrap_sampler, In.Uvs);
    float4 rock  = txRock.Sample(linear_wrap_sampler, In.Uvs);
    float4 sand  = txSand.Sample(linear_wrap_sampler, In.Uvs);
    float4 layer = txLayer.Sample(linear_wrap_sampler, In.Uvs / texture_scale); 
    
    color = lerp(color, rock, layer.r);
    color = lerp(color, sand,  layer.g);
    color = lerp(color, grass, layer.b);
    
    color *= 0.7f;
    
    if (ocean_active) color.rgb *= 0.5 + 0.5 * max(0, min(1, In.PosWS.y * 0.5 + 0.5));

    return PackGBuffer(color.xyz, normalize(In.NormalVS), float3(0, 0, 0),
    1.0f, 0.0f, 1.0f); 
}

