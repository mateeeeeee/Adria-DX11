#include "ParticleGlobals.hlsli"


// Particle buffer in two parts
RWStructuredBuffer<GPUParticlePartA> ParticleBufferA : register(u0);
RWStructuredBuffer<GPUParticlePartB> ParticleBufferB : register(u1);

// The dead list, so any particles that are retired this frame can be added to this list
AppendStructuredBuffer<uint> DeadListToAddTo : register(u2);

// The alive list which gets built using this shader
RWStructuredBuffer<float2> IndexBuffer : register(u3);

// Viewspace particle positions are calculated here and stored
RWStructuredBuffer<float4> ViewSpacePositions : register(u4);

// The draw args for the DrawInstancedIndirect call needs to be filled in before the rasterization path is called, so do it here
RWBuffer<uint> DrawArgs : register(u5);

// The opaque scene's depth buffer read as a texture
//Texture2D DepthBuffer : register(t0); for collisions maybe later

[numthreads(256, 1, 1)]
void main( uint3 id : SV_DispatchThreadID )
{
    // Initialize the draw args using the first thread in the Dispatch call
    if (id.x == 0)
    {
        DrawArgs[0] = 0; // Number of primitives reset to zero
        DrawArgs[1] = 1; // Number of instances is always 1
        DrawArgs[2] = 0;
        DrawArgs[3] = 0;
        DrawArgs[4] = 0;
    }

    // Wait after draw args are written so no other threads can write to them before they are initialized
    GroupMemoryBarrierWithGroupSync();

    const float3 Gravity = float3(0.0, -9.81, 0.0);

	// Fetch the particle from the global buffer
    GPUParticlePartA pa = ParticleBufferA[id.x];
    GPUParticlePartB pb = ParticleBufferB[id.x];

    if (pb.Age > 0.0f)
    {
        // Age the particle by counting down from Lifespan to zero
        pb.Age -= delta_time;

        // Update the rotation
        pa.Rotation += 0.24 * delta_time;

        float3 NewPosition = pb.Position;

        // Apply force due to gravity
        if (pa.IsSleeping == 0)
        {
            pb.Velocity += pb.Mass * Gravity * delta_time;

			// Apply a little bit of a wind force
            float3 windDir = float3(wind_direction_x, wind_direction_y, 0);
            float windStrength = 0.1;

            pb.Velocity += normalize(windDir) * windStrength * delta_time;
			
			// Calculate the new position of the particle
            NewPosition += pb.Velocity * delta_time;
        }

        // Calculate the normalized age
        float fScaledLife = 1.0 - saturate(pb.Age / pb.Lifespan);
		
		// Calculate the size of the particle based on age
        float radius = lerp(pb.StartSize, pb.EndSize, fScaledLife);
		
		// By default, we are not going to kill the particle
        bool killParticle = false;

        //check for collisions later here

        // Put particle to sleep if the velocity is small
        if (length(pb.Velocity) < 0.01)
        {
            pa.IsSleeping = 1;
        }

		// If the position is below the floor, let's kill it now rather than wait for it to retire
        if (NewPosition.y < -10)
        {
            killParticle = true;
        }

        // Write the new position
        pb.Position = NewPosition;

		// Calculate the the distance to the eye for sorting in the rasterization path
        float3 vec = NewPosition - camera_position.xyz;
        pb.DistanceToEye = length(vec);

		// The opacity is a function of the age
        float alpha = lerp(1, 0, saturate(fScaledLife - 0.8) / 0.2);
        pa.TintAndAlpha.a = pb.Age <= 0 ? 0 : alpha;
        pa.TintAndAlpha.rgb = float3(1, 1, 1);

        // Pack the view spaced position and radius into a float4 buffer
        float4 viewSpacePositionAndRadius;

        viewSpacePositionAndRadius.xyz = mul(float4(NewPosition, 1), view).xyz;
        viewSpacePositionAndRadius.w = radius;

        ViewSpacePositions[id.x] = viewSpacePositionAndRadius;
        // Dead particles are added to the dead list for recycling
        if (pb.Age <= 0.0f || killParticle)
        {
            pb.Age = -1;
            DeadListToAddTo.Append(id.x);
        }
        else
        {
			// Alive particles are added to the alive list
            uint index = IndexBuffer.IncrementCounter();
            IndexBuffer[index] = float2(pb.DistanceToEye, (float) id.x);
            uint dstIdx = 0;
            InterlockedAdd(DrawArgs[0], 6, dstIdx);
        }
    }

    // Write the particle data back to the global particle buffer
    ParticleBufferA[id.x] = pa;
    ParticleBufferB[id.x] = pb;
}