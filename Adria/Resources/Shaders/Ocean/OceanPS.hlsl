#include "../Globals/GlobalsPS.hlsli"


Texture2D NormalMap     : register(t0);
TextureCube SkyCube     : register(t1); 
Texture2D FoamTexture   : register(t2);



struct PS_INPUT
{
    float4 Position     : SV_POSITION;
    float4 WorldPos     : POS;
    float2 TexCoord     : TEX;
};

float4 main(PS_INPUT IN) : SV_TARGET
{
   
    float4 NormalFoam = NormalMap.Sample(linear_wrap_sampler, IN.TexCoord);
    
    float FoamFactor = NormalFoam.a;
    
    float3 n = normalize(NormalFoam.xyz);
    
    float3 vdir = camera_position.xyz - IN.WorldPos.xyz;

    float3 v = normalize(vdir);
    float3 l = reflect(-v, n);

    float F0 = 0.020018673;
    float F = F0 + (1.0 - F0) * pow(1.0 - dot(n, l), 5.0);

    float3 sky_color = SkyCube.Sample(linear_wrap_sampler, l).xyz;

    float3 ocean_color = diffuse;

    float3 sky = F * sky_color;
    float dif = clamp(dot(n, normalize(light_dir.xyz)), 0.f, 1.f);
    float3 water = (1.f - F) * ocean_color * sky_color * dif;
    
    water += FoamFactor * FoamTexture.Sample(linear_wrap_sampler, IN.TexCoord).rgb; //+perlin_offset

    float3 color = sky + water;

    float spec = pow(clamp(dot(normalize(light_dir.xyz), l), 0.0, 1.0), 128.0);

    return float4(color + spec * light_color.xyz, 0.75f);
}


/*

#define PERLIN 0

#if PERLIN 
Texture2D Perlin        : register(t3);
#define BLEND_START		800		
#define BLEND_END		20000		

static const float3 perlinFrequency = float3(1, 2, 4);
static const float3 perlinAmplitude = float3(0.8, 0.4, 0.2);
static const float3 perlinGradient = float3(0.014, 0.016, 0.022);

#endif

#if PERLIN

    float dist = length(vdir.xz);
    float factor = (BLEND_END - dist) / (BLEND_END - BLEND_START);
    float2 perl = float2(0.0, 0.0f);

    factor = clamp(factor * factor * factor, 0.0, 1.0);

    float2 perlin_offset = float2(perlin_offset_x, perlin_offset_y);
    if (factor < 1.0)
    {
        float2 p0 = Perlin.Sample(LinearWrapSampler, IN.TexCoord * perlinFrequency.x + perlin_offset );
        float2 p1 = Perlin.Sample(LinearWrapSampler, IN.TexCoord * perlinFrequency.y + perlin_offset );               
        float2 p2 = Perlin.Sample(LinearWrapSampler, IN.TexCoord * perlinFrequency.z + perlin_offset );

        perl = (p0 * perlinGradient.x + p1 * perlinGradient.y + p2 * perlinGradient.z);
    }
    
    // calculate thingies
    grad.xz = lerp(perl, grad.xz, factor);

#endif*/