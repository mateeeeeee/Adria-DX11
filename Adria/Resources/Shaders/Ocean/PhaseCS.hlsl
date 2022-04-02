#include "../Globals/GlobalsCS.hlsli"


#define COMPUTE_WORK_GROUP_DIM 32

static const float PI = 3.14159265359;
static const float g = 9.81;
static const float KM = 370.0;


Texture2D<float4>   Phases      : register(t0);
RWTexture2D<float4> DeltaPhases : register(u0);



float Omega(float k)
{
    return sqrt(g * k * (1.0 + k * k / KM * KM));
}

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

[numthreads(COMPUTE_WORK_GROUP_DIM, COMPUTE_WORK_GROUP_DIM, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel_coord = dispatchID.xy;

    float n = (pixel_coord.x < 0.5f * uint(resolution)) ? pixel_coord.x : pixel_coord.x - float(resolution);
    float m = (pixel_coord.y < 0.5f * uint(resolution)) ? pixel_coord.y : pixel_coord.y - float(resolution);

    float2 wave_vector = (2.f * PI * float2(n, m)) / ocean_size;
    float k = length(wave_vector);

    // Note: An ad-hoc factor to make the phase change per-frame slow
    float slowdown_factor = 1.f;

    float delta_phase = Omega(k) * delta_time * slowdown_factor;
    float phase = Phases.Load(uint3(pixel_coord, 0)).r;
   
    phase = mod(phase + delta_phase, 2.f * PI);
    
    DeltaPhases[pixel_coord] = float4(phase, 0.f, 0.f, 0.f);
}