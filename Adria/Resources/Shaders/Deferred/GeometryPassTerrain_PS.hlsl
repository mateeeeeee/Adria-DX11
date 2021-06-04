#include "../Globals/GlobalsPS.hlsli"



Texture2D txAlbedo : register(t0);



struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
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
 
    float4 DiffuseColor = txAlbedo.Sample(linear_wrap_sampler, In.Uvs) * albedo_factor;

    if (DiffuseColor.a < 0.1)
        discard;

    float3 Normal = normalize(In.NormalWS);

    return PackGBuffer(DiffuseColor.xyz, normalize(In.NormalVS), float3(0, 0, 0),
    1.0f, 0.0f, 1.0f); 

}