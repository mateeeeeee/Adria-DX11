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
    float4 Position     : SV_POSITION;
    float4 ClipSpacePos : POSITION;
    matrix InverseModel : INVERSE_MODEL;
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

    float2 screen_pos = input.ClipSpacePos.xy / input.ClipSpacePos.w;
    float2 depth_coords = screen_pos * float2(0.5f, -0.5f) + 0.5f;
    float depth = txDepth.Sample(point_clamp_sampler, depth_coords).r;

    float4 posVS = float4(GetPositionVS(depth_coords, depth), 1.0f);
    float4 posWS = mul(posVS, inverse_view);
    float4 posLS = mul(posWS, input.InverseModel);
    posLS.xyz /= posLS.w;

    clip(0.5f - abs(posLS.xyz));

    float2 tex_coords = posLS.xy + 0.5f;
    float4 albedo = txAlbedoDecal.Sample(linear_wrap_sampler, tex_coords);
    if (albedo.a < 0.1) discard;
    pout.DiffuseRoughness.rgb = albedo.rgb;
    return pout;
}