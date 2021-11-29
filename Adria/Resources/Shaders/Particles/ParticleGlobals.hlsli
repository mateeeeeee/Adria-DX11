
#include "../Globals/GlobalsCS.hlsli"

struct GPUParticlePartA
{
    float4 TintAndAlpha;    // The color and opacity
    float Rotation;         // The rotation angle
    uint IsSleeping;        // Whether or not the particle is sleeping (ie, don't update position)
};

struct GPUParticlePartB
{
    float3 Position;    // World space position
    float Mass;         // Mass of particle

    float3 Velocity;    // World space velocity
    float Lifespan;     // Lifespan of the particle.

    float DistanceToEye; // The distance from the particle to the eye
    float Age;           // The current age counting down from lifespan to zero
    float StartSize;     // The size at spawn time
    float EndSize;       // The time at maximum age
};

// The number of dead particles in the system
cbuffer DeadListCountCBuffer : register(b11)
{
    uint NumDeadParticles;
};


// The number of alive particles this frame
cbuffer ActiveListCountCBuffer : register(b12)
{
    uint NumActiveParticles;
};

