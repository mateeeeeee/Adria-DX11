#include "../Globals/GlobalsVS.hlsli"

struct GPUParticlePartA
{
    float4 TintAndAlpha; // The color and opacity
    float Rotation; // The rotation angle
    uint IsSleeping; // Whether or not the particle is sleeping (ie, don't update position)
};

// The particle buffer data. Note this is only one half of the particle data - the data that is relevant to rendering as opposed to simulation
StructuredBuffer<GPUParticlePartA> ParticleBufferA : register(t0);

// A buffer containing the pre-computed view space positions of the particles
StructuredBuffer<float4> ViewSpacePositions : register(t1);

// The sorted index list of particles
StructuredBuffer<float2> IndexBuffer : register(t2);

// The number of alive particles this frame
cbuffer ActiveListCountCBuffer : register(b12)
{
    uint NumActiveParticles;
};

struct PS_INPUT
{
    float4 ViewSpaceCentreAndRadius : TEXCOORD0;
    float2 TexCoord                 : TEXCOORD1;
    float3 ViewPos                  : TEXCOORD2;
    float4 Color                    : COLOR0;
    float4 Position                 : SV_POSITION;
};

PS_INPUT main( uint VertexId : SV_VertexID )
{
    PS_INPUT Output = (PS_INPUT) 0;

	// Particle index 
    uint particleIndex = VertexId / 4;

	// Per-particle corner index
    uint cornerIndex = VertexId % 4;

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

    float4 ViewSpaceCentreAndRadius = ViewSpacePositions[index];

    float2 offset = offsets[cornerIndex];
    float2 uv = (offset + 1) * float2(0.25, 0.5);

    float radius = ViewSpaceCentreAndRadius.w;
    float3 cameraFacingPos;

    float s, c;
    sincos(pa.Rotation, s, c);
    float2x2 rotation = { float2(c, -s), float2(s, c) };
		
    offset = mul(offset, rotation);

    cameraFacingPos = ViewSpaceCentreAndRadius.xyz;
    cameraFacingPos.xy += radius * offset;

    Output.Position = mul(float4(cameraFacingPos, 1), projection);
    Output.TexCoord = uv;
    Output.Color = pa.TintAndAlpha;
    Output.ViewSpaceCentreAndRadius = ViewSpaceCentreAndRadius;
    Output.ViewPos = cameraFacingPos;
	
    return Output;

}