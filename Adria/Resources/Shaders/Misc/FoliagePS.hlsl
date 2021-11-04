
#include "../Globals/GlobalsPS.hlsli"


Texture2D txDiffuse : register(t0);


struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEX;
    float3 Normal : NORMAL;
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


PS_GBUFFER_OUT main(PS_INPUT IN)
{
    IN.TexCoord.y = 1 - IN.TexCoord.y;
    float4 texColor = txDiffuse.Sample(linear_wrap_sampler, IN.TexCoord) * float4(diffuse, 1.0) * albedo_factor;
    if (texColor.a < 0.5f) discard;
    
    return PackGBuffer(texColor.xyz, normalize(IN.Normal), float3(0, 0, 0),
    0.0f, 0.0f, 1.0f);
}