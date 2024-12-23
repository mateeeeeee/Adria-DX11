#include <Common.hlsli>

#define COMPUTE_WORK_GROUP_DIM 32

static const float PI = 3.14159265359f;
static const float g = 9.81f;
static const float KM = 370.0f;
static const float CM = 0.23f;

RWTexture2D<float4> InitialSpectrumTx : register(u0);

float Omega(float k)
{
    return sqrt(g * k * (1.f + ((k * k) / (KM * KM))));
}
float Square(float x)
{
    return x * x;
}

[numthreads(COMPUTE_WORK_GROUP_DIM, COMPUTE_WORK_GROUP_DIM, 1)]
void InitialSpectrumCS(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchID.xy;
    int resolution = computeData.resolution;
    float n = (pixelCoord.x < 0.5f * uint(resolution)) ? pixelCoord.x : pixelCoord.x - float(resolution);
    float m = (pixelCoord.y < 0.5f * uint(resolution)) ? pixelCoord.y : pixelCoord.y - float(resolution);

    float2 waveVector = (2.f * PI * float2(n, m)) / computeData.oceanSize;
    float k = length(waveVector);
    float U10 = length(float2(computeData.windDirectionX, computeData.windDirectionY));

    float omega = 0.84f;
    float kp = g * Square(omega / U10);
    float c = Omega(k) / k;
    float cp = Omega(kp) / kp;
	
    float Lpm = exp(-1.25 * Square(kp / k));
    const float GAMMA = 1.7;
    float sigma = 0.08 * (1.0 + 4.0 * pow(omega, -3.0));
    float gamma = exp(-Square(sqrt(k / kp) - 1.0) / 2.0 * Square(sigma));
    float Jp = pow(GAMMA, gamma);
    float Fp = Lpm * Jp * exp(-omega / sqrt(10.0) * (sqrt(k / kp) - 1.0));
    float alphap = 0.006 * sqrt(omega);
    float Bl = 0.5 * alphap * cp / c * Fp;
	
    float z0 = 0.000037 * Square(U10) / g * pow(U10 / cp, 0.9);
    float uStar = 0.41 * U10 / log(10.0 / z0);
    float alpham = 0.01 * ((uStar < CM) ? (1.0 + log(uStar / CM)) : (1.0 + 3.0 * log(uStar / CM)));
    float Fm = exp(-0.25 * Square(k / KM - 1.0));
    float Bh = 0.5 * alpham * CM / c * Fm * Lpm;
	
    float a0 = log(2.0) / 4.0;
    float am = 0.13 * uStar / CM;
    float delta = tanh(a0 + 4.0 * pow(c / cp, 2.5) + am * pow(CM / c, 2.5));
	
    float cosPhi = dot(normalize(float2(computeData.windDirectionX, computeData.windDirectionY)), normalize(waveVector));
    float S = (1.0 / (2.0 * PI)) * pow(k, -4.0) * (Bl + Bh) * (1.0 + delta * (2.0 * cosPhi * cosPhi - 1.0));
	
    float dk = 2.0 * PI / computeData.oceanSize;
    float h = sqrt(S / 2.0) * dk;
	
    if (waveVector.x == 0.0 && waveVector.y == 0.0)
        h = 0.f;
    
    InitialSpectrumTx[pixelCoord] = float4(h, 0.f, 0.f, 0.f);

}