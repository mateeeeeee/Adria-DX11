#include "../Globals/GlobalsPS.hlsli"

struct PS_INPUT
{
    float4 ViewSpaceCentreAndRadius : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
    float3 ViewPos : TEXCOORD2;
    float4 Color : COLOR0;
    float4 Position : SV_POSITION;
};

Texture2D ParticleTexture : register(t0);

Texture2D<float> DepthTexture : register(t1);


float4 main(PS_INPUT In) : SV_TARGET
{
    float3 particleViewSpacePos = In.ViewSpaceCentreAndRadius.xyz;
    float particleRadius = In.ViewSpaceCentreAndRadius.w;

    float depth = DepthTexture.Load(uint3(In.Position.x, In.Position.y, 0)).x;

    float uv_x = In.Position.x / screen_resolution.x;
    float uv_y = 1 - (In.Position.y / screen_resolution.y);


    float3 viewSpacePos = GetPositionVS(float2(uv_x, uv_y), depth);

    if (particleViewSpacePos.z > viewSpacePos.z)
    {
        clip(-1);
    }

    float depthFade = saturate((viewSpacePos.z - particleViewSpacePos.z) / particleRadius);

    float4 albedo = 1.0f;
    albedo.a = depthFade;
    albedo *= ParticleTexture.SampleLevel(linear_clamp_sampler, In.TexCoord, 0);
    float4 color = albedo * In.Color;

    return color;
}