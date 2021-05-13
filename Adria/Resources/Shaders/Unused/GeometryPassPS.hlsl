
#include "../Globals/GlobalsPS.hlsli"


#if HAS_DIFFUSE_TEXTURE

Texture2D txDiffuse : register(t0);

#endif

#if HAS_SPECULAR_TEXTURE

Texture2D txSpecular : register(t1);

#endif


#if HAS_NORMAL_TEXTURE

Texture2D txNormal : register(t2);

#endif




struct VS_OUTPUT
{
    float4 Position : SV_POSITION; // vertex position 
#if HAS_DIFFUSE_TEXTURE || HAS_SPECULAR_TEXTURE
    float2 Uvs : TEX;
#endif
    float3 NormalVS : NORMAL0; // vertex normal
#if HAS_NORMAL_TEXTURE
    float3 TangentWS    : TANGENT;
    float3 BitangentWS  : BITANGENT;
    float3 NormalWS     : NORMAL1;
    matrix view_matrix  : MATRIX0;
#endif
};




struct PS_GBUFFER_OUT
{
    float4 NormalSpecIntensity  : SV_TARGET0;
    float4 DiffuseSpecPow       : SV_TARGET1;
};


PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float3 NormalVS, float SpecIntensity, float SpecPower)
{
    PS_GBUFFER_OUT Out;

	// Normalize the specular power
    float SpecPowerNorm = log2(SpecPower) / 10.5f;
    Out.NormalSpecIntensity = float4(0.5 * NormalVS + 0.5, SpecIntensity);
    Out.DiffuseSpecPow = float4(BaseColor, SpecPowerNorm);
    return Out;
}




PS_GBUFFER_OUT ps_main(VS_OUTPUT In)
{

#if HAS_DIFFUSE_TEXTURE || HAS_SPECULAR_TEXTURE

    In.Uvs.y = 1 - In.Uvs.y;

#endif   
    
#if HAS_DIFFUSE_TEXTURE
    
    float4 DiffuseColor = txDiffuse.Sample(linear_wrap_sampler,  In.Uvs) * float4( diffuse, 1.0);

    if(DiffuseColor.a < 0.9) discard;
#else 
    float4 DiffuseColor = float4(diffuse, 1.0);
#endif

    
#if HAS_SPECULAR_TEXTURE
 
    float SpecIntensity = txSpecular.Sample(linear_wrap_sampler,  In.Uvs).r * specular.r;
#else 
    float SpecIntensity = specular.r;
#endif
    

#if HAS_NORMAL_TEXTURE
    float3 Normal = normalize(In.NormalWS);
    float3 Tangent = normalize(In.TangentWS);
    //Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    
    float3 Bitangent = normalize(In.BitangentWS); //cross(Tangent, Normal); 
    
    float3 BumpMapNormal = txNormal.Sample( linear_wrap_sampler, In.Uvs );
    BumpMapNormal = 2.0f*BumpMapNormal - 1.0f;
    
    float3 NewNormal;
    float3x3 TBN = float3x3(Tangent, Bitangent, Normal);
   
    NewNormal = mul(BumpMapNormal, TBN);
  
     //In.NormalWS = NewNormal;
     In.NormalVS = normalize(mul(NewNormal, (float3x3)In.view_matrix));
#endif
    

    return PackGBuffer(DiffuseColor.xyz, normalize(In.NormalVS), SpecIntensity, shininess);
}