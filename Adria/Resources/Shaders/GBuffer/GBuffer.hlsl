#include <Common.hlsli>

struct VSInput
{
    float3 Position : POSITION; 
    float2 Uvs      : TEX;
    float3 Normal   : NORMAL;
    float3 Tan      : TANGENT;
    float3 Bitan    : BITANGENT;
};

struct VSToPS
{
    float4 Position     : SV_POSITION;
    float2 Uvs          : TEX;
    float3 NormalVS     : NORMAL0; 

    float3 TangentWS    : TANGENT;
    float3 BitangentWS  : BITANGENT;
    float3 NormalWS     : NORMAL1;
};


VSToPS GBufferVS(VSInput input)
{
    VSToPS Output = (VSToPS)0;
    
    float4 pos = mul(float4(input.Position, 1.0), objectData.model);
    Output.Position = mul(pos, frameData.viewprojection);
    Output.Position.xy += frameData.cameraJitter * Output.Position.w;
    Output.Uvs = input.Uvs;

    float3 worldSpaceNormal = mul(input.Normal, (float3x3) objectData.transposedInverseModel);
    Output.NormalVS = mul(worldSpaceNormal, (float3x3) transpose(frameData.inverseView));
    Output.TangentWS = mul(input.Tan, (float3x3) objectData.model);
    Output.BitangentWS = mul(input.Bitan, (float3x3) objectData.model);
    Output.NormalWS = worldSpaceNormal;

    return Output;
}

Texture2D<float4> AlbedoTx   : register(t0);      
Texture2D<float4> MetallicRoughnessTx : register(t1);
Texture2D<float4> NormalTx   : register(t2);
Texture2D<float4> EmissiveTx : register(t3);

struct PSOutput
{
    float4 NormalMetallic   : SV_TARGET0;
    float4 DiffuseRoughness : SV_TARGET1;
    float4 Emissive         : SV_TARGET2;
};


PSOutput PackGBuffer(float3 baseColor, float3 viewSpaceNormal, float4 emissive, float roughness, float metallic)
{
    PSOutput Out = (PSOutput)0;
    Out.NormalMetallic = float4(0.5 * viewSpaceNormal + 0.5, metallic);
    Out.DiffuseRoughness = float4(baseColor, roughness);
    Out.Emissive = float4(emissive.xyz, emissive.w / 256);
    return Out;
}

PSOutput GBufferPS(VSToPS input, bool IsFrontFace : SV_IsFrontFace)
{
    input.Uvs.y = 1 - input.Uvs.y;
    float4 albedoColor = AlbedoTx.Sample(LinearWrapSampler, input.Uvs) * materialData.albedoFactor;

#ifdef MASK
    if(albedoColor.a < materialData.alphaCutoff) discard;
#endif

    float3 normal = normalize(input.NormalWS);
    if (!IsFrontFace) normal = -normal;
    
    float3 tangent = normalize(input.TangentWS);
    float3 bitangent = normalize(input.BitangentWS); 
    float3 bumpMapNormal = NormalTx.Sample( LinearWrapSampler, input.Uvs );
    bumpMapNormal = 2.0f * bumpMapNormal - 1.0f;
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    float3 newNormal = mul(bumpMapNormal, TBN);
    float3 viewSpaceNormal = normalize(mul(newNormal, (float3x3)frameData.view));

    float3 aoRoughnessMetallic = MetallicRoughnessTx.Sample(LinearWrapSampler, input.Uvs).rgb;
    float3 emissiveColor = EmissiveTx.Sample(LinearWrapSampler, input.Uvs).rgb;
    return PackGBuffer(albedoColor.xyz, normalize(viewSpaceNormal), float4(emissiveColor,  materialData.emissiveFactor),
    aoRoughnessMetallic.g *  materialData.roughnessFactor, aoRoughnessMetallic.b *  materialData.metallicFactor);
}