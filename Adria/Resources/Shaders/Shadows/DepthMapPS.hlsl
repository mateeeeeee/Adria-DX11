

#if TRANSPARENT
SamplerState linear_wrap_sampler : register(s0);
Texture2D texDiff : register(t0);
#endif

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
#if TRANSPARENT
    float2 TexCoords : TEX;
#endif
};


void ps_main(VS_OUTPUT pin)//: SV_Target
{
#if TRANSPARENT //can also use clip
    if( texDiff.Sample(linear_wrap_sampler,pin.TexCoords).a < 0.1 ) 
        discard;
#endif
}