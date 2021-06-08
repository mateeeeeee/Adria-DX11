#include "../Globals/GlobalsPS.hlsli"
#include "../Util/DitherUtil.hlsli"

Texture2D scene_texture : register(t0);


static const int NUM_SAMPLES = 32;

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEX;
};




float4 main(VertexOut pin) : SV_TARGET
{
    float2 tex_coord = pin.Tex;
    float3 color = scene_texture.SampleLevel(linear_clamp_sampler, tex_coord, 0).rgb;
    
    float2 light_pos = current_light.screen_space_position.xy;
    
    float2 delta_tex_coord = (tex_coord - light_pos);
    delta_tex_coord *= current_light.godrays_density / NUM_SAMPLES;
    
    float illumination_decay = 1.0f;
  
    float3 accumulated_god_rays = float3(0.0f,0.0f,0.0f);

    float3 accumulated = 0.0f;
    
    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        tex_coord.xy -= delta_tex_coord;
        float3 sam = scene_texture.SampleLevel(linear_clamp_sampler, tex_coord.xy, 0).rgb;
        sam *= illumination_decay * current_light.godrays_weight;
        accumulated += sam;
        illumination_decay *= current_light.godrays_decay;
    }
    
    accumulated *= current_light.godrays_exposure;
    
    return float4(color + accumulated, 1.0f);
}
