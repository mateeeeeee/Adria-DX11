#include "../Globals/GlobalsPS.hlsli"

Texture2D<float4> txAlbedo   : register(t0);      
Texture2D<float4> txMetallicRoughness : register(t1);
Texture2D<float4> txNormal   : register(t2);
Texture2D<float4> txEmissive : register(t3);

struct VSOutput
{
    float4 Position     : SV_POSITION; 
    float2 Uvs          : TEX;
    float3 NormalVS     : NORMAL0; 
    float3 TangentWS    : TANGENT;
    float3 BitangentWS  : BITANGENT;
    float3 NormalWS     : NORMAL1;
    bool  IsFrontFace   : SV_IsFrontFace;
};

struct PSOutput
{
    float4 NormalMetallic   : SV_TARGET0;
    float4 DiffuseRoughness : SV_TARGET1;
    float4 Emissive         : SV_TARGET2;
};


PSOutput PackGBuffer(float3 BaseColor, float3 NormalVS, float4 emissive, float roughness, float metallic)
{
    PSOutput Out;

    Out.NormalMetallic = float4(0.5 * NormalVS + 0.5, metallic);
    Out.DiffuseRoughness = float4(BaseColor, roughness);
    Out.Emissive = float4(emissive.xyz, emissive.w / 256);
    return Out;
}

PSOutput main(VSOutput In)
{
    In.Uvs.y = 1 - In.Uvs.y;
    float4 DiffuseColor = txAlbedo.Sample(linear_wrap_sampler, In.Uvs) * albedo_factor;

#ifdef MASK
    if(DiffuseColor.a < alpha_cutoff) discard;
#endif

    float3 Normal = normalize(In.NormalWS);
    if (!In.IsFrontFace) Normal = -Normal;
    
    float3 Tangent = normalize(In.TangentWS);
    float3 Bitangent = normalize(In.BitangentWS); 
    float3 BumpMapNormal = txNormal.Sample( linear_wrap_sampler, In.Uvs );
    BumpMapNormal = 2.0f*BumpMapNormal - 1.0f;
    float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
    float3 NewNormal = mul(BumpMapNormal, TBN);
    In.NormalVS = normalize(mul(NewNormal, (float3x3)view));

    float3 ao_roughness_metallic = txMetallicRoughness.Sample(linear_wrap_sampler, In.Uvs).rgb;
    
    float3 EmissiveColor = txEmissive.Sample(linear_wrap_sampler, In.Uvs).rgb;
    return PackGBuffer(DiffuseColor.xyz, normalize(In.NormalVS), float4(EmissiveColor, emissive_factor),
    ao_roughness_metallic.g * roughness_factor, ao_roughness_metallic.b * metallic_factor);
}