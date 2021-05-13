
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

    float3 red = float3(0.393f, 0.769f, 0.189f);
    float3 green = float3(0.349f, 0.686f, 0.168f);
    float3 blue = float3(0.272f, 0.534f, 0.131f);

    float3 output;
    output.r = dot(color.rgb, red);
    output.g = dot(color.rgb, green);
    output.b = dot(color.rgb, blue);
    return float4(output, color.a);
}
