#include <Common.hlsli>

struct VSInput
{
    float3 Pos : POSITION;
    float2 Uvs : TEX;
    float3 Normal : NORMAL;
    float3 Offset : INSTANCE_OFFSET;
    float  RotationY : INSTANCE_ROTATION;
};

struct VSToPS
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEX;
    float3 Normal   : NORMAL;
};

matrix RotationAroundYAxis(float angle)
{
    return float4x4(cos(angle), 0.0f, -sin(angle), 0.0f,
             0.0f,       1.0f, 0.0f,       0.0f,
             sin(angle), 0.0f, cos(angle), 0.0f,
             0.0f,       0.0f, 0.0f,       1.0f);
}



VSToPS FoliageVS(VSInput input)
{
    VSToPS output = (VSToPS)0;
    matrix modelMatrix = mul(RotationAroundYAxis(input.RotationY), objectData.model);
    float3 position = input.Pos;
    position += input.Offset;
    modelMatrix[3].xyz += input.Offset;
    
    if(input.Pos.y > 0.75f)
    {
        float xWindOffset = (2 * sin(weatherData.time + input.Pos.x + input.Pos.y + input.Pos.z) + 1) * weatherData.windSpeed * weatherData.windDir.x * 0.005f;
        float zWindOffset = (1 * sin(2 * (weatherData.time + input.Pos.x + input.Pos.y + input.Pos.z)) + 0.5) * weatherData.windSpeed * weatherData.windDir.z * 0.005f;
        modelMatrix[3].xyz += float3(xWindOffset, 0, zWindOffset);
    }
    
    matrix modelView = mul(modelMatrix, frameData.view);
    float4 viewSpacePosition = mul(float4(input.Pos, 1.0), modelView);
    output.Position = mul(viewSpacePosition, frameData.projection);
    output.TexCoord = input.Uvs;
    
    float3 worldSpaceNormal = float3(0.0, 1, 0.0);        
    output.Normal = mul(worldSpaceNormal, (float3x3) transpose(frameData.inverseView));
    return output;
}

Texture2D FoliageTx : register(t0);


struct PSOutput
{
    float4 NormalMetallic : SV_TARGET0;
    float4 DiffuseRoughness : SV_TARGET1;
    float4 EmissiveAO : SV_TARGET2;
};

PSOutput PackGBuffer(float3 BaseColor, float3 NormalVS, float3 emissive, float roughness, float metallic, float ao)
{
    PSOutput Out;

    Out.NormalMetallic = float4(0.5 * NormalVS + 0.5, metallic);
    Out.DiffuseRoughness = float4(BaseColor, roughness);
    Out.EmissiveAO = float4(emissive, ao);
    return Out;
}


PSOutput FoliagePS(VSToPS input)
{
    input.TexCoord.y = 1 - input.TexCoord.y;
    float4 texColor = FoliageTx.Sample(LinearWrapSampler, input.TexCoord) * float4(materialData.diffuse, 1.0) * materialData.albedoFactor;
    if (texColor.a < 0.5f) discard;
    
    return PackGBuffer(texColor.xyz, normalize(input.Normal), float3(0, 0, 0),
    0.0f, 0.0f, 1.0f);
}