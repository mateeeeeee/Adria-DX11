#include <Common.hlsli>
#include "ParticleUtil.hlsli"


RWStructuredBuffer<GPUParticlePartA> ParticleBufferA : register(u0);
RWStructuredBuffer<GPUParticlePartB> ParticleBufferB : register(u1);

AppendStructuredBuffer<uint> DeadListToAddTo : register(u2);
RWStructuredBuffer<float2> IndexBuffer : register(u3);
RWStructuredBuffer<float4> ViewSpacePositions : register(u4);
RWBuffer<uint> DrawArgs : register(u5);

Texture2D DepthBuffer : register(t0);

float3 CalcViewSpacePositionFromDepth(float2 normalizedScreenPosition, int2 texelOffset);

[numthreads(256, 1, 1)]
void ParticleSimulateCS( uint3 id : SV_DispatchThreadID )
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
        pb.Age -= computeData.deltaTime;
        pa.Rotation += 0.24 * computeData.deltaTime;
        float3 newPosition = pb.Position;

        if (pa.IsSleeping == 0)
        {
            pb.Velocity += pb.Mass * Gravity * computeData.deltaTime;
			float3 windDir = float3(computeData.windDirectionX, computeData.windDirectionX, 0);
            float windLength = length(windDir);
            const float windStrength = 0.1;
            if (windLength > 0.0f) pb.Velocity += windDir / windLength * windStrength * computeData.deltaTime;
			newPosition += pb.Velocity * computeData.deltaTime;
        }

        float scaledLife = 1.0 - saturate(pb.Age / pb.Lifespan);
		float radius = lerp(pb.StartSize, pb.EndSize, scaledLife);
		
		bool killParticle = false;
        if (Collisions)
        {
			float3 viewSpaceParticlePosition = mul(float4(newPosition, 1), frameData.view).xyz;
			float4 screenSpaceParticlePosition = mul(float4(newPosition, 1), frameData.viewprojection);
            screenSpaceParticlePosition.xyz /= screenSpaceParticlePosition.w;

			if (pa.IsSleeping == 0 && screenSpaceParticlePosition.x > -1 && screenSpaceParticlePosition.x < 1 && screenSpaceParticlePosition.y > -1 && screenSpaceParticlePosition.y < 1)
            {
				float3 viewSpacePosOfDepthBuffer = CalcViewSpacePositionFromDepth(screenSpaceParticlePosition.xy, int2(0, 0));
                if ((viewSpaceParticlePosition.z > viewSpacePosOfDepthBuffer.z) && (viewSpaceParticlePosition.z < viewSpacePosOfDepthBuffer.z + CollisionThickness)) 
                {
					float3 surfaceNormal;

					float3 p0 = viewSpacePosOfDepthBuffer;
                    float3 p1 = CalcViewSpacePositionFromDepth(screenSpaceParticlePosition.xy, int2(1, 0));
                    float3 p2 = CalcViewSpacePositionFromDepth(screenSpaceParticlePosition.xy, int2(0, 1));

					float3 viewSpaceNormal = normalize(cross(p2 - p0, p1 - p0));
					surfaceNormal = normalize(mul(-viewSpaceNormal, frameData.inverseView).xyz);
					float3 newVelocity = reflect(pb.Velocity, surfaceNormal);
					pb.Velocity = 0.3 * newVelocity;
					newPosition = pb.Position + (pb.Velocity * computeData.deltaTime);
                }
            }
        }

        if (length(pb.Velocity) < 0.01)
        {
            pa.IsSleeping = 1;
        }

		if (newPosition.y < -10)
        {
            killParticle = true;
        }

        pb.Position = newPosition;

		float3 vec = newPosition - frameData.cameraPosition.xyz;
        pb.DistanceToEye = length(vec);

		float alpha = lerp(1, 0, saturate(scaledLife - 0.8) / 0.2);
        pa.TintAndAlpha.a = pb.Age <= 0 ? 0 : alpha;
        pa.TintAndAlpha.rgb = float3(1, 1, 1);

        float4 viewSpacePositionAndRadius;
        viewSpacePositionAndRadius.xyz = mul(float4(newPosition, 1), frameData.view).xyz;
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


float3 CalcViewSpacePositionFromDepth(float2 normalizedScreenPosition, int2 texelOffset)
{
	normalizedScreenPosition.x += (float) texelOffset.x / (float) frameData.screenResolution.x;
    normalizedScreenPosition.y += (float) texelOffset.y / (float) frameData.screenResolution.y;

    float2 uv;
	uv.x = (0.5 + normalizedScreenPosition.x * 0.5) * (float) frameData.screenResolution.x;
    uv.y = (1 - (0.5 + normalizedScreenPosition.y * 0.5)) * (float) frameData.screenResolution.y;

	float depth = DepthBuffer.Load(uint3(uv.x, uv.y, 0)).x;
	float4 viewSpacePosOfDepthBuffer;
    viewSpacePosOfDepthBuffer.xy = normalizedScreenPosition.xy;
    viewSpacePosOfDepthBuffer.z = depth;
    viewSpacePosOfDepthBuffer.w = 1;

	viewSpacePosOfDepthBuffer = mul(viewSpacePosOfDepthBuffer, frameData.inverseProjection);
    viewSpacePosOfDepthBuffer.xyz /= viewSpacePosOfDepthBuffer.w;
    return viewSpacePosOfDepthBuffer.xyz;
}