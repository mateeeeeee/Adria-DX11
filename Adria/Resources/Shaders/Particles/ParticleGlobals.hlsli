
#include "../Globals/GlobalsCS.hlsli"

struct GPUParticlePartA
{
    float4 TintAndAlpha;    
    float Rotation;         
    uint IsSleeping;        
};

struct GPUParticlePartB
{
    float3 Position;   
    float Mass;        

    float3 Velocity;   
    float Lifespan;    

    float DistanceToEye;
    float Age;         
    float StartSize;   
    float EndSize;     
};


cbuffer DeadListCountCBuffer : register(b11)
{
    uint NumDeadParticles;
};

cbuffer ActiveListCountCBuffer : register(b12)
{
    uint NumActiveParticles;
};

