#include "../Globals/GlobalsPS.hlsli"

struct PS_INPUT
{
    float4 ViewSpaceCentreAndRadius : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
    float3 ViewPos : TEXCOORD2;
    float4 Color : COLOR0;
    float4 Position : SV_POSITION;
};

// The texture atlas for the particles
Texture2D ParticleTexture : register(t0);

// The opaque scene depth buffer read as a texture
Texture2D<float> DepthTexture : register(t1);


float4 main(PS_INPUT In) : SV_TARGET
{
	// Retrieve the particle data
    float3 particleViewSpacePos = In.ViewSpaceCentreAndRadius.xyz;
    float particleRadius = In.ViewSpaceCentreAndRadius.w;

	// Get the depth at this point in screen space
    float depth = DepthTexture.Load(uint3(In.Position.x, In.Position.y, 0)).x;

    float uv_x = In.Position.x / screen_resolution.x;
    float uv_y = 1 - (In.Position.y / screen_resolution.y);


    float3 viewSpacePos = GetPositionVS(float2(uv_x, uv_y), depth);

    // remove this?
    if (particleViewSpacePos.z > viewSpacePos.z)
    {
        clip(-1);
    }

	// Calculate the depth fade factor
    float depthFade = saturate((viewSpacePos.z - particleViewSpacePos.z) / particleRadius);

    float4 albedo = 1;
    albedo.a = depthFade;
    albedo *= ParticleTexture.SampleLevel(linear_clamp_sampler, In.TexCoord, 0); // 2d
	// Multiply in the particle color
    float4 color = albedo * In.Color;
	
	// Calculate the UV based the screen space position
    float3 n = 0;
    float2 uv = (In.ViewPos.xy - particleViewSpacePos.xy) / particleRadius;
    // Scale and bias
    uv = (1 + uv) * 0.5;
    const float pi = 3.1415926535897932384626433832795;
    n.x = -cos(pi * uv.x);
    n.y = -cos(pi * uv.y);
    n.z = sin(pi * length(uv));
    n = normalize(n);
    float ndotl = saturate(dot(light_dir.xyz, n)); //check if light_dir is in VS

    // Ambient lighting plus directional lighting
    float3 lighting = (ambient_color + ndotl * light_color).xyz;

	// Multiply lighting term in
    color.rgb *= lighting;
    return color;
}