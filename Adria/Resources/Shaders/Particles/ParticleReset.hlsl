#include "ParticleUtil.hlsli"

RWStructuredBuffer<GPUParticlePartA>	ParticleBufferA		: register(u0);
RWStructuredBuffer<GPUParticlePartB>	ParticleBufferB		: register(u1);

[numthreads(256, 1, 1)]
void ParticleResetCS(uint3 id : SV_DispatchThreadID)
{
	ParticleBufferA[id.x] = (GPUParticlePartA)0;
	ParticleBufferB[id.x] = (GPUParticlePartB)0;
}