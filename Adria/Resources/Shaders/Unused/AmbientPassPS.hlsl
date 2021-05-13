
#include "../Globals/GlobalsPS.hlsli"


Texture2D    diffuseTx : register(t1);
#if SSAO
Texture2D<float> ssaoTx : register(t7);
#endif

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEX;
};

float4 ps_main(VertexOut pin) : SV_TARGET
{
    float4 texColor = diffuseTx.Sample(linear_wrap_sampler, pin.Tex);
    float ao = 1.0f;
#if SSAO
    ao = ssaoTx.Sample(linear_wrap_sampler, pin.Tex);
#endif
    return float4(global_ambient.xyz * texColor.xyz * ao, 1.0f);

}