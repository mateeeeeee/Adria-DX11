#include <Common.hlsli>
#include <Util/LightUtil.hlsli>

Texture2D NormalMetallicTx  : register(t0);
Texture2D DiffuseTx         : register(t1);
Texture2D<float> DepthTx    : register(t2);
Texture2D EmissiveTx        : register(t3);


#if SSAO
Texture2D<float> AmbientOcclusionTx : register(t7);
#endif

#if IBL
TextureCube SpecularTexture : register(t8);
TextureCube IrradianceTexture : register(t9);
Texture2D SpecularBRDF_LUT : register(t10);

uint QuerySpecularTextureLevels()
{
    uint width, height, levels;
    SpecularTexture.GetDimensions(0, width, height, levels);
    return levels;
}
#endif


struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

float4 AmbientPS(VSToPS input) : SV_TARGET
{
    float4 albedoRoughness = DiffuseTx.Sample(AnisotropicSampler, input.Tex);
    float3 albedo = albedoRoughness.rgb;
    float4 emissiveData = EmissiveTx.Sample(AnisotropicSampler, input.Tex);
    float emissiveFactor = emissiveData.a * 256.0f;
    float3 emissive = emissiveData.rgb * emissiveFactor;
    float ao = 1.0f; 

#if SSAO
    ao = AmbientOcclusionTx.Sample(AnisotropicSampler, input.Tex);
#endif
    

#if IBL
    float3 ibl = float3(0.0f, 0.0f, 0.0f);
    float4 normalMetallic = NormalMetallicTx.Sample(AnisotropicSampler, input.Tex);
    float3 viewNormal = 2 * normalMetallic.rgb - 1.0;
    float metallic = normalMetallic.a;
    
    float depth = DepthTx.Sample(AnisotropicSampler, input.Tex);
    float3 viewPosition = GetViewSpacePosition(input.Tex, depth);
    float4 worldPosition = mul(float4(viewPosition, 1.0f), frameData.inverseView);
    worldPosition /= worldPosition.w;

    float3 V = normalize(frameData.cameraPosition.xyz - worldPosition.xyz);
    float roughness = albedoRoughness.a;
    float3 worldNormal = mul(viewNormal, (float3x3) transpose(frameData.view));
	{
        float cosLo = max(0.0, dot(worldNormal, V));
		float3 irradiance = IrradianceTexture.Sample(AnisotropicSampler, worldNormal).rgb;

        float3 F0 = float3(0.04, 0.04, 0.04);
        F0 = lerp(F0, albedo.rgb, metallic);
		float3 F = FresnelSchlickRoughness(cosLo, F0, roughness);
        float3 kd = 1.0 - F;
        kd *= 1.0 - metallic;
       
		float3 diffuseIBL = kd * albedo.rgb * irradiance;
        uint specularTextureLevels = QuerySpecularTextureLevels();
        float3 Lr = reflect(-V, worldNormal); 
       
        const float MAX_REFLECTION_LOD = min(4.0, specularTextureLevels);
        float3 specularIrradiance = SpecularTexture.SampleLevel(AnisotropicSampler, Lr, roughness * MAX_REFLECTION_LOD).rgb;
        float2 specularBRDF = SpecularBRDF_LUT.Sample(LinearClampSampler, float2(cosLo, roughness)).rg;
        float3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;
        ibl = diffuseIBL + specularIBL;
    }
    
    return float4(ibl, 1.0f) * ao + float4(emissive.rgb, 1.0f);
#else
    return frameData.globalAmbient * float4(albedo, 1.0f) * ao + float4(emissive.rgb, 1.0f);
#endif
}