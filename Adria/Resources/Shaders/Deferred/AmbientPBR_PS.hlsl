#include "../Globals/GlobalsPS.hlsli"

Texture2D normalMetallicTx  : register(t0);
Texture2D diffuseTx         : register(t1);
Texture2D<float> depthTx    : register(t2);
Texture2D emissiveTx        : register(t3);



#if SSAO
Texture2D<float> ssaoTx : register(t7);
#endif

#if IBL

TextureCube specularTexture : register(t8);
TextureCube irradianceTexture : register(t9);
Texture2D specularBRDF_LUT : register(t10);


uint querySpecularTextureLevels()
{
    uint width, height, levels;
    specularTexture.GetDimensions(0, width, height, levels);
    return levels;
}

#endif




struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEX;
};

float4 main(VertexOut pin) : SV_TARGET
{
    float4 albedo_roughness = diffuseTx.Sample(anisotropic_sampler, pin.Tex);
    float3 albedo = albedo_roughness.rgb;
    float4 emissive_data = emissiveTx.Sample(anisotropic_sampler, pin.Tex);
    float emissive_factor = emissive_data.a * 256.0f;
    float3 emissive = emissive_data.rgb * emissive_factor;
    float ao = 1.0f; 
    
    
#if SSAO
    ao = ssaoTx.Sample(anisotropic_sampler, pin.Tex);
#endif
    

#if IBL
     //1 - emissiveAo.a
    // Ambient lighting (IBL).
    float3 ibl = float3(0.0f, 0.0f, 0.0f);
    float4 NormalMetallic = normalMetallicTx.Sample(anisotropic_sampler, pin.Tex);
    float3 ViewNormal = 2 * NormalMetallic.rgb - 1.0;
    
    float metallic = NormalMetallic.a;
    
    float Depth = depthTx.Sample(anisotropic_sampler, pin.Tex);
    
    float3 ViewPosition = GetPositionVS(pin.Tex, Depth);
    
    float4 WorldPosition = mul(float4(ViewPosition, 1.0f), inverse_view);
    
    WorldPosition /= WorldPosition.w;

    float3 V = normalize(camera_position.xyz - WorldPosition.xyz);

    float roughness = albedo_roughness.a;
    
    //Normal = mul(Normal, inverse_view);
    
    float3 WorldNormal = mul(ViewNormal, (float3x3) transpose(view));
    
    
    
	{

        float cosLo = max(0.0, dot(WorldNormal, V));
        
		// Sample diffuse irradiance at normal direction.
        float3 irradiance = irradianceTexture.Sample(anisotropic_sampler, WorldNormal).rgb;

        float3 F0 = float3(0.04, 0.04, 0.04);
        F0 = lerp(F0, albedo.rgb, metallic);
		// Calculate Fresnel term for ambient lighting.
		// Since we use pre-filtered cubemap(s) and irradiance is coming from many directions
		// use cosLo instead of angle with light's half-vector (cosLh above).
		// See: https://seblagarde.wordpress.com/2011/08/17/hello-world/
        float3 F = fresnelSchlickRoughness(cosLo, F0, roughness);
        //
		//// Get diffuse contribution factor (as with direct lighting).
        float3 kd = 1.0 - F;
        kd *= 1.0 - metallic;
       
		//// Irradiance map contains exitant radiance assuming Lambertian BRDF, no need to scale by 1/PI here either.
        float3 diffuseIBL = kd * albedo.rgb * irradiance;
        //
		//// Sample pre-filtered specular reflection environment at correct mipmap level.
        uint specularTextureLevels = querySpecularTextureLevels();
        float3 Lr = reflect(-V, WorldNormal); 
       
        const float MAX_REFLECTION_LOD = min(4.0, specularTextureLevels);
        
        float3 specularIrradiance = specularTexture.SampleLevel(anisotropic_sampler, Lr, roughness * MAX_REFLECTION_LOD).rgb;

		// Split-sum approximation factors for Cook-Torrance specular BRDF.
        float2 specularBRDF = specularBRDF_LUT.Sample(linear_clamp_sampler, float2(cosLo, roughness)).rg;

		// Total specular IBL contribution.
        float3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

		// Total ambient lighting contribution.
        ibl = diffuseIBL + specularIBL;
    }
    
    return float4(ibl, 1.0f) * ao + float4(emissive.rgb, 1.0f);
#else
    return global_ambient * float4(albedo, 1.0f) * ao + float4(emissive.rgb, 1.0f);
#endif
}