#include <Common.hlsli>
#include "ParticleUtil.hlsli"

StructuredBuffer<GPUParticlePartA> ParticleBufferA : register(t0);
StructuredBuffer<float4> ViewSpacePositions : register(t1);
StructuredBuffer<float2> IndexBuffer : register(t2);

struct VSToPS
{
    float4 ViewSpaceCentreAndRadius : TEXCOORD0;
    float2 TexCoord                 : TEXCOORD1;
    float3 ViewPos                  : TEXCOORD2;
    float4 Color                    : COLOR0;
    float4 Position                 : SV_POSITION;
};

VSToPS ParticleVS(uint vertexId : SV_VertexID)
{
    VSToPS output = (VSToPS)0;

	uint particleIndex = vertexId / 4;
	uint cornerIndex = vertexId % 4;

    float xOffset = 0;
    const float2 offsets[4] =
    {
        float2(-1, 1),
		float2(1, 1),
		float2(-1, -1),
		float2(1, -1),
    };

    uint index = (uint) IndexBuffer[NumActiveParticles - particleIndex - 1].y;
    GPUParticlePartA pa = ParticleBufferA[index];

    float4 viewSpaceCentreAndRadius = ViewSpacePositions[index];
    float2 offset = offsets[cornerIndex];
    float2 uv = (offset + 1) * float2(0.5, 0.5);

    float radius = viewSpaceCentreAndRadius.w;
    float3 cameraFacingPos;

    float s, c;
    sincos(pa.Rotation, s, c);
    float2x2 rotation = { float2(c, -s), float2(s, c) };
    offset = mul(offset, rotation);

    cameraFacingPos = viewSpaceCentreAndRadius.xyz;
    cameraFacingPos.xy += radius * offset;

    output.Position = mul(float4(cameraFacingPos, 1), frameData.projection);
    output.TexCoord = uv;
    output.Color = pa.TintAndAlpha;
    output.ViewSpaceCentreAndRadius = viewSpaceCentreAndRadius;
    output.ViewPos = cameraFacingPos;
    return output;
}


Texture2D ParticleTexture : register(t0);
Texture2D<float> DepthTexture : register(t1);


float4 ParticlePS(VSToPS input) : SV_TARGET
{
    float3 particleViewSpacePos = input.ViewSpaceCentreAndRadius.xyz;
    float particleRadius = input.ViewSpaceCentreAndRadius.w;
    float depth = DepthTexture.Load(uint3(input.Position.x, input.Position.y, 0)).x;

    float uvX = input.Position.x / frameData.screenResolution.x;
    float uvY = 1 - (input.Position.y / frameData.screenResolution.y);

    float3 viewSpacePosition = GetViewSpacePosition(float2(uvX, uvY), depth);
    if (particleViewSpacePos.z > viewSpacePosition.z)
    {
        clip(-1);
    }

    float depthFade = saturate((viewSpacePosition.z - particleViewSpacePos.z) / particleRadius);
    float4 albedo = 1.0f;
    albedo.a = depthFade;
    albedo *= ParticleTexture.SampleLevel(LinearClampSampler, input.TexCoord, 0);
    float4 color = albedo * input.Color;
    return color;
}