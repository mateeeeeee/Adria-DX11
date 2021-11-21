
#include "ParticleGlobals.hlsli"

// A texture filled with random values for generating some variance in our particles when we spawn them
Texture2D								RandomBuffer			: register(t0);

// The particle buffers to fill with new particles
RWStructuredBuffer<GPUParticlePartA>	ParticleBufferA		: register(u0);
RWStructuredBuffer<GPUParticlePartB>	ParticleBufferB		: register(u1);

// The dead list interpretted as a consume buffer. So every time we consume an index from this list, it automatically decrements the atomic counter (ie the number of dead particles)
ConsumeStructuredBuffer<uint>			DeadListToAllocFrom	: register(u2);


cbuffer EmitterCBuffer : register(b13)
{
	float4	EmitterPosition;
	float4	EmitterVelocity;
	float4	PositionVariance;

	int		MaxParticlesThisFrame;
	float	ParticleLifeSpan;
	float	StartSize;
	float	EndSize;

	float	VelocityVariance;
	float	Mass;
    float	elapsedTime;
};


// Emit particles, one per thread, in blocks of 1024 at a time
[numthreads(1024, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	// Check to make sure we don't emit more particles than we specified
    if (id.x < NumDeadParticles && id.x < MaxParticlesThisFrame)
	{
		// Initialize the particle data to zero to avoid any unexpected results
		GPUParticlePartA pa = (GPUParticlePartA)0;
		GPUParticlePartB pb = (GPUParticlePartB)0;

		// Generate some random numbers from reading the random texture
        float2 uv = float2(id.x / 1024.0, elapsedTime);
		float3 randomValues0 = RandomBuffer.SampleLevel(linear_wrap_sampler, uv, 0).xyz;

		float velocityMagnitude = length(EmitterVelocity.xyz);

		pa.Rotation = 0;
		pa.IsSleeping = 0;
		
		pb.Mass = Mass;
        pb.Velocity = EmitterVelocity.xyz + (randomValues0.xyz * velocityMagnitude * VelocityVariance);
		pb.Lifespan = ParticleLifeSpan;
		pb.Age = pb.Lifespan;
		pb.StartSize = StartSize;
		pb.EndSize = EndSize;

		// The index into the global particle list obtained from the dead list. 
		// Calling consume will decrement the counter in this buffer.
		uint index = DeadListToAllocFrom.Consume();

		// Write the new particle state into the global particle buffer
		ParticleBufferA[index] = pa;
		ParticleBufferB[index] = pb;
	}
}