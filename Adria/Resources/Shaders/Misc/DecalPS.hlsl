#include "../Globals/GlobalsPS.hlsli"

Texture2D<float4> txAlbedoDecal : register(t0);
Texture2D<float4> txNormalDecal : register(t1);
Texture2D<float>  txDepth       : register(t2);

struct PS_DECAL_OUT
{
    float4 DiffuseRoughness : SV_TARGET0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
};

cbuffer DecalCBuffer : register(b11)
{
    row_major matrix decal_projection;
    row_major matrix decal_viewprojection;
    row_major matrix decal_inverse_viewprojection;
    float2 aspect_ratio;
}

PS_DECAL_OUT main(PS_INPUT input)
{
    PS_DECAL_OUT pout = (PS_DECAL_OUT) 0;

    float2 screen_pos = input.Position.xy / screen_resolution;
    float2 tex_coords = screen_pos * float2(0.5, -0.5) + 0.5;
    float depth = txDepth.Sample(point_clamp_sampler, tex_coords).x;
    float3 view_position = GetPositionVS(screen_pos, depth);
    float4 ndc = mul(float4(view_position, 1.0f), decal_projection);
    ndc.xyz /= ndc.w;
    ndc.xy *= aspect_ratio;
    if (ndc.x < -1.0 || ndc.x > 1.0 || ndc.y < -1.0 || ndc.y > 1.0) discard;
    float2 decal_uvs = ndc.xy * 0.5 + 0.5;
    decal_uvs.y = 1.0 - decal_uvs.y;

    float4 albedo = txAlbedoDecal.Sample(linear_wrap_sampler, decal_uvs);
    if (albedo.a < 0.1) discard;
    pout.DiffuseRoughness.rgb = albedo.rgb;
    return pout;
}