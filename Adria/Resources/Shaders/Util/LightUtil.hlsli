#ifndef _LIGHT_UTIL_
#define _LIGHT_UTIL_

#include <CommonData.hlsli>

struct LightingResult
{
    float4 diffuse;
    float4 specular;
};

//for structured buffer
struct StructuredLight
{
    float4 position;
    float4 direction;
    float4 color;
    int active;
    float range;
    int type;
    float outerCosine;
    float innerCosine;
    int castsShadows;
    int useCascades;
    int padd;
};

Light CreateLightFromStructured(in StructuredLight structured_light)
{
    Light l = (Light)0;
    l.castsShadows = structured_light.castsShadows;
    l.color = structured_light.color;
    l.direction = structured_light.direction;
    l.innerCosine = structured_light.innerCosine;
    l.outerCosine = structured_light.outerCosine;
    l.position = structured_light.position;
    l.range = structured_light.range;
    l.type = structured_light.type;
    l.useCascades = structured_light.useCascades;
    
    l.godraysDecay = 0;
    l.godraysDensity = 0;
    l.godraysExposure = 0;
    l.godraysWeight = 0;
    l.screenSpacePosition = 0;
    l.screenSpaceShadows = 0;
    l.volumetricStrength = 0;
    
    return l;
}


#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2


float DoAttenuation(float distance, float range)
{
    float att = saturate(1.0f - (distance * distance / (range * range)));
    return att * att;
}

float4 DoDiffuse(Light light, float3 L, float3 N)
{
    float NdotL = max(0, dot(N, L));
    return light.color * NdotL;
}

float4 DoSpecular(Light light, float shininess, float3 L, float3 N, float3 V)
{
    float3 R = normalize(reflect(-L, N));
    float RdotV = max(0.0001, dot(R, V));
    return light.color * pow(RdotV, shininess);
}

LightingResult DoPointLight(Light light, float shininess, float3 V, float3 P, float3 N)
{
    LightingResult result;
    light.position.xyz /= light.position.w;
 
    float3 L = light.position.xyz - P;
    float distance = length(L);
    L = L / distance;
    
    N = normalize(N);
    float attenuation = DoAttenuation(distance, light.range);
 
    result.diffuse = DoDiffuse(light, L, N) * attenuation;
    result.specular = DoSpecular(light, shininess, V, L, N) * attenuation;
    return result;
}

LightingResult DoDirectionalLight(Light light, float shininess, float3 V, float3 N)
{
    LightingResult result;
    N = normalize(N);
    float3 L = -light.direction.xyz;
    L = normalize(L);
 
    result.diffuse = DoDiffuse(light, L, N);
    result.specular = DoSpecular(light, shininess, V, L, N);
    return result;
}

LightingResult DoSpotLight(Light light, float shininess, float3 V, float3 P, float3 N)
{
    LightingResult result;
    
    N = normalize(N);
    float3 L = light.position.xyz - P;
    float distance = length(L);
    L = L / distance;
 
    float attenuation = DoAttenuation(distance, light.range);
    float3 lightDir = normalize(light.direction.xyz);
    float cosAng = dot(-lightDir, L);
    
    float conAtt = saturate((cosAng - light.innerCosine) / (light.outerCosine - light.innerCosine));
    conAtt *= conAtt;

    result.diffuse = DoDiffuse(light, L, N) * attenuation * conAtt;
    result.specular = DoSpecular(light, shininess, V, L, N) * attenuation * conAtt;
    return result;
}

static const float PI = 3.14159265359;
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001); 
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float3 DoSpotLightPBR(Light light, float3 positionVS, float3 normalVS, float3 V, float3 albedo, float metallic, float roughness)
{
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);
    
    float3 L = normalize(light.position.xyz - positionVS);
    float3 H = normalize(V + L);
    float distance = length(light.position.xyz - positionVS);
    float attenuation = DoAttenuation(distance, light.range);
    
    float3 lightDir = normalize(light.direction.xyz);
    float cosAng = dot(-lightDir, L);
    float conAtt = saturate((cosAng - light.outerCosine) / (light.innerCosine - light.outerCosine));
    conAtt *= conAtt;
    
    float3 radiance = light.color.xyz * attenuation * conAtt;

    float NDF = DistributionGGX(normalVS, H, roughness);
    float G = GeometrySmith(normalVS, V, L, roughness);
    float3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
           
    float3 nominator = NDF * G * F;
    float denominator = 4 * max(dot(normalVS, V), 0.0) * max(dot(normalVS, L), 0.0);
    float3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0

    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;
    float NdotL = max(dot(normalVS, L), 0.0);
    float3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    return Lo;
}

float3 DoPointLightPBR(Light light, float3 positionVS, float3 normalVS, float3 V, float3 albedo, float metallic, float roughness)
{
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);
    
    float3 L = normalize(light.position.xyz - positionVS);
    float3 H = normalize(V + L);
    float distance = length(light.position.xyz - positionVS);
    float attenuation = DoAttenuation(distance, light.range);
    float3 radiance = light.color.xyz * attenuation;
    
    float NDF = DistributionGGX(normalVS, H, roughness);
    float G = GeometrySmith(normalVS, V, L, roughness);
    float3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
           
    float3 nominator = NDF * G * F;
    float denominator = 4 * max(dot(normalVS, V), 0.0) * max(dot(normalVS, L), 0.0);
    float3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0
        
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;
    float NdotL = max(dot(normalVS, L), 0.0);
    float3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    return Lo;
}

float3 DoDirectionalLightPBR(Light light, float3 positionVS, float3 normalVS, float3 V, float3 albedo, float metallic, float roughness)
{
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);

    float3 L = normalize(-light.direction.xyz);
    float3 H = normalize(V + L);
    
    float3 radiance = light.color.xyz;

    float NDF = DistributionGGX(normalVS, H, roughness);
    float G = GeometrySmith(normalVS, V, L, roughness);
    float3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
           
    float3 nominator = NDF * G * F;
    float denominator = 4 * max(dot(normalVS, V), 0.0) * max(dot(normalVS, L), 0.0);
    float3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0

    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;
    float NdotL = max(dot(normalVS, L), 0.0);
    float3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    return Lo;
}

#endif