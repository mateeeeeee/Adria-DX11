#include "../Globals/GlobalsPS.hlsli"
#include "../Util/ToneMapUtil.hlsli"

Texture2D hdr_texture : register(t0);

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEX;
};


float4 main(VertexOut pin) : SV_TARGET
{
    
    float4 color = hdr_texture.Sample(linear_wrap_sampler, pin.Tex);
    
#if REINHARD
    float4 tone_mapped_color = float4(ReinhardToneMapping(color.xyz * tone_map_exposure), 1.0);
#elif LINEAR
    float4 tone_mapped_color = float4(LinearToneMapping(color.xyz * tone_map_exposure), 1.0);
#elif HABLE 
    float4 tone_mapped_color = float4(HableToneMapping(color.xyz * tone_map_exposure), 1.0);
#else 
    float4 tone_mapped_color = float4(color.xyz * tone_map_exposure, 1.0f);
#endif
    return tone_mapped_color;

}