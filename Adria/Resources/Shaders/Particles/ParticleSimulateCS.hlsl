#include "ParticleGlobals.hlsli"


RWStructuredBuffer<GPUParticlePartA> ParticleBufferA : register(u0);
RWStructuredBuffer<GPUParticlePartB> ParticleBufferB : register(u1);

AppendStructuredBuffer<uint> DeadListToAddTo : register(u2);

RWStructuredBuffer<float2> IndexBuffer : register(u3);

RWStructuredBuffer<float4> ViewSpacePositions : register(u4);

RWBuffer<uint> DrawArgs : register(u5);

//Texture2D DepthBuffer : register(t0); for collisions maybe later

[numthreads(256, 1, 1)]
void main( uint3 id : SV_DispatchThreadID )
{
    if (id.x == 0)
    {
        DrawArgs[0] = 0; 
        DrawArgs[1] = 1; 
        DrawArgs[2] = 0;
        DrawArgs[3] = 0;
        DrawArgs[4] = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    const float3 Gravity = float3(0.0, -9.81, 0.0);

	GPUParticlePartA pa = ParticleBufferA[id.x];
    GPUParticlePartB pb = ParticleBufferB[id.x];

    if (pb.Age > 0.0f)
    {
        pb.Age -= delta_time;

        pa.Rotation += 0.24 * delta_time;

        float3 NewPosition = pb.Position;

        if (pa.IsSleeping == 0)
        {
            pb.Velocity += pb.Mass * Gravity * delta_time;

			float3 windDir = float3(wind_direction_x, wind_direction_y, 0);
            float windLength = length(windDir);
            const float windStrength = 0.1;
            if (windLength > 0.0f) pb.Velocity += windDir / windLength * windStrength * delta_time;
			
			NewPosition += pb.Velocity * delta_time;
        }

        float fScaledLife = 1.0 - saturate(pb.Age / pb.Lifespan);
		
		float radius = lerp(pb.StartSize, pb.EndSize, fScaledLife);
		
		bool killParticle = false;

        if (length(pb.Velocity) < 0.01)
        {
            pa.IsSleeping = 1;
        }

		if (NewPosition.y < -10)
        {
            killParticle = true;
        }

        pb.Position = NewPosition;

		float3 vec = NewPosition - camera_position.xyz;
        pb.DistanceToEye = length(vec);

		float alpha = lerp(1, 0, saturate(fScaledLife - 0.8) / 0.2);
        pa.TintAndAlpha.a = pb.Age <= 0 ? 0 : alpha;
        pa.TintAndAlpha.rgb = float3(1, 1, 1);

        float4 viewSpacePositionAndRadius;

        viewSpacePositionAndRadius.xyz = mul(float4(NewPosition, 1), view).xyz;
        viewSpacePositionAndRadius.w = radius;

        ViewSpacePositions[id.x] = viewSpacePositionAndRadius;
        if (pb.Age <= 0.0f || killParticle)
        {
            pb.Age = -1;
            DeadListToAddTo.Append(id.x);
        }
        else
        {
            uint index = IndexBuffer.IncrementCounter();
            IndexBuffer[index] = float2(pb.DistanceToEye, (float) id.x);
            uint dstIdx = 0;
            InterlockedAdd(DrawArgs[0], 6, dstIdx);
        }
    }

    ParticleBufferA[id.x] = pa;
    ParticleBufferB[id.x] = pb;
}