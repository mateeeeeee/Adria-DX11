#include "../Globals/GlobalsPS.hlsli"


Texture2D txGrass   : register(t0);
Texture2D txSnow    : register(t1);
Texture2D txRock    : register(t2);
Texture2D txSand    : register(t3);

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
 
    float4 grass = txGrass.Sample(linear_wrap_sampler, In.Uvs);
    float4 snow  = txSnow.Sample(linear_wrap_sampler,  In.Uvs);
    float4 rock  = txRock.Sample(linear_wrap_sampler,  In.Uvs);
    float4 sand  = txSand.Sample(linear_wrap_sampler,  In.Uvs);

    float4 color = 0.0f;
    float3 normal = normalize(In.NormalWS);

    float angleDiff = abs(dot(normal.xyz, float3(0, 1, 0)));
    float pureRock = 0.6;
    float lerpRock = 0.7;
    float coef = 1.0 - smoothstep(pureRock, lerpRock, angleDiff);
    grass = lerp(grass, rock, coef);
    snow = lerp(snow, rock, coef);

    float height = In.PosWS.y;
    
    if (height > snow_height + mix_zone) 
    {
        color = snow;
    }
    else if (height > snow_height - mix_zone)
    {
        float coef = (height - (snow_height - mix_zone)) / (2.0 * mix_zone);
        color = lerp(grass, snow, coef);
    }
    else if (height > grass_height + mix_zone)
    {
        color = grass;
    }
    else if (height > grass_height - mix_zone)
    {
        float coef = (height - (grass_height - mix_zone)) / (2.0 * mix_zone);
        color = lerp(sand, grass, coef);
    }
    else
    {
        color = sand;
    }
    

    return PackGBuffer(color.xyz, normalize(In.NormalVS), float3(0, 0, 0),
    1.0f, 0.0f, 1.0f); 

}