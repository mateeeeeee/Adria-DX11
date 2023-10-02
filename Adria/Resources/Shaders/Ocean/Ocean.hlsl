#include <Common.hlsli>

struct VSInput
{
    float3 Pos : POSITION;
    float2 Uvs : TEX;
};

struct VSToPS
{
    float4 Position     : SV_POSITION;
    float4 WorldPos     : POS;
    float2 TexCoord     : TEX;
};

Texture2D DisplacementTx : register(t0);
static const float LAMBDA = 1.2f;


VSToPS OceanVS(VSInput input)
{
    VSToPS output = (VSToPS)0;
    float4 worldPosition = mul(float4(input.Pos, 1.0), objectData.model);
    worldPosition /= worldPosition.w;
    
    float3 dx = DisplacementTx.SampleLevel(LinearWrapSampler, input.Uvs, 0.0f).xyz;
    worldPosition.xyz += LAMBDA * dx;

    output.Position   = mul(worldPosition, frameData.viewprojection);
    output.TexCoord   = input.Uvs; 
    output.WorldPos   = worldPosition;
    return output;                       
}


Texture2D NormalTx      : register(t0);
TextureCube SkyCubeTx   : register(t1); 
Texture2D FoamTx        : register(t2);

float4 OceanPS(VSToPS input) : SV_TARGET
{
    float4 normalFoam = NormalTx.Sample(LinearWrapSampler, input.TexCoord);   
    float FoamFactor = normalFoam.a;
    float3 N = normalize(normalFoam.xyz);
    
    float3 V = frameData.cameraPosition.xyz - input.WorldPos.xyz;
    V = normalize(V);
    float3 L = reflect(-V, N);

    float F0 = 0.020018673;
    float F = F0 + (1.0 - F0) * pow(1.0 - dot(N, L), 5.0);

    float3 skyColor = SkyCubeTx.Sample(LinearWrapSampler, L).xyz;
    float3 oceanColor = materialData.diffuse;

    float3 sky = F * skyColor;
    float dif = clamp(dot(N, normalize(weatherData.lightDir.xyz)), 0.f, 1.f);
    float3 water = (1.f - F) * oceanColor * skyColor * dif;
    
    water += FoamFactor * FoamTx.Sample(LinearWrapSampler, input.TexCoord).rgb; 
    float3 color = sky + water;

    float spec = pow(clamp(dot(normalize(weatherData.lightDir.xyz), L), 0.0, 1.0), 128.0);
    return float4(color + spec * weatherData.lightColor.xyz, 0.75f);
}

