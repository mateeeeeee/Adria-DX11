
SamplerState linear_wrap_sampler : register(s0);
Texture2D<float4> Texture : register(t0);

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEX;
};


float4 main(VertexOut pin) : SV_Target0
{
    float4 color = Texture.Sample(linear_wrap_sampler, pin.Tex);
    float3 grayscale = float3(0.2125f, 0.7154f, 0.0721f);
    float3 output = dot(color.rgb, grayscale);
    return float4(output, color.a);
}