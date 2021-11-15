// Particle buffer in two parts

struct GPUParticlePartA
{
	float4	TintAndAlpha;			// The color and opacity

	float2	VelocityXY;				// View space velocity XY used for streak extrusion
	float	EmitterNdotL;			// The lighting term for the while emitter
	uint	EmitterProperties;		// The index of the emitter in 0-15 bits, 16-23 for atlas index, 24th bit is whether or not the emitter supports velocity-based streaks

	float	Rotation;				// The rotation angle
	uint	IsSleeping;				// Whether or not the particle is sleeping (ie, don't update position)
	uint	CollisionCount;			// Keep track of how many times the particle has collided
	float	pads[1];
};

struct GPUParticlePartB
{
	float3	Position;				// World space position
	float	Mass;					// Mass of particle

	float3	Velocity;				// World space velocity
	float	Lifespan;				// Lifespan of the particle.

	float	DistanceToEye;			// The distance from the particle to the eye
	float	Age;					// The current age counting down from lifespan to zero
	float	StartSize;				// The size at spawn time
	float	EndSize;				// The time at maximum age
};

RWStructuredBuffer<GPUParticlePartA>	ParticleBufferA		: register(u0);
RWStructuredBuffer<GPUParticlePartB>	ParticleBufferB		: register(u1);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	g_ParticleBufferA[id.x] = (GPUParticlePartA)0;
	g_ParticleBufferB[id.x] = (GPUParticlePartB)0;
}