#include <Common.hlsli>

struct VSInput
{
    float3 Pos : POSITION;
};

struct VSToPS
{
    float4 Position : SV_POSITION;
    float4 ClipSpacePos : POSITION;
    matrix InverseModel : INVERSE_MODEL;
};

VSToPS DecalVS(VSInput input)
{
    VSToPS output = (VSToPS)0;
    float4 worldPosition = mul(float4(input.Pos, 1.0f), objectData.model);
    output.Position = mul(worldPosition, frameData.viewprojection);
    output.ClipSpacePos = output.Position;
    output.InverseModel = transpose(objectData.transposedInverseModel);
    return output;
}

Texture2D<float4> decalAlbedoTx : register(t0);
Texture2D<float4> decalNormalTx : register(t1);
Texture2D<float>  depthTx       : register(t2);

struct PSOutput
{
    float4 DiffuseRoughness : SV_TARGET0;
#ifdef DECAL_MODIFY_NORMALS
    float4 NormalMetallic   : SV_TARGET1;
#endif
};

#define DECAL_XY 0
#define DECAL_YZ 1
#define DECAL_XZ 2


cbuffer DecalCBuffer : register(b11)
{
    int decalType;
}

PSOutput DecalPS(VSToPS input)
{
    PSOutput output = (PSOutput) 0;

    float2 screenPosition = input.ClipSpacePos.xy / input.ClipSpacePos.w;
    float2 depthCoords = screenPosition * float2(0.5f, -0.5f) + 0.5f;
    float depth = depthTx.Sample(PointClampSampler, depthCoords).r;

    float4 viewSpacePosition = float4(GetViewSpacePosition(depthCoords, depth), 1.0f);
    float4 worldSpacePosition = mul(viewSpacePosition, frameData.inverseView);
    float4 localSpacePosition = mul(worldSpacePosition, input.InverseModel);
    localSpacePosition.xyz /= localSpacePosition.w;

    clip(0.5f - abs(localSpacePosition.xyz));

    float2 texCoords = 0.0f;
    switch (decalType)
    {
        case DECAL_XY:
            texCoords = localSpacePosition.xy + 0.5f;
            break;
        case DECAL_YZ:
            texCoords = localSpacePosition.yz + 0.5f;
            break;
        case DECAL_XZ:
            texCoords = localSpacePosition.xz + 0.5f;
            break;
        default:
            output.DiffuseRoughness.rgb = float3(1, 0, 0);
            return output;
    }

    float4 albedo = decalAlbedoTx.SampleLevel(LinearWrapSampler, texCoords, 0);
    if (albedo.a < 0.1) discard;
    output.DiffuseRoughness.rgb = albedo.rgb;

#ifdef DECAL_MODIFY_NORMALS
    worldSpacePosition /= worldSpacePosition.w;
    float3 ddxWorldSpace = ddx(worldSpacePosition.xyz);
    float3 ddyWorldSpace = ddy(worldSpacePosition.xyz);

    float3 normal   = normalize(cross(ddxWorldSpace, ddyWorldSpace));
    float3 binormal = normalize(ddxWorldSpace);
    float3 tangent  = normalize(ddyWorldSpace);
    float3x3 TBN = float3x3(tangent, binormal, normal);

    float3 decalNormal = decalNormalTx.Sample(LinearWrapSampler, texCoords);
    decalNormal = 2.0f * decalNormal - 1.0f;
    decalNormal = mul(decalNormal, TBN);
    float3 DecalNormalVS = normalize(mul(decalNormal, (float3x3)frameData.view));
    output.NormalMetallic.rgb = 0.5 * DecalNormalVS + 0.5;
#endif
    return output;
}