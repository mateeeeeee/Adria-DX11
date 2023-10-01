#include <Common.hlsli>

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float3 Dir : POSITION;
};

float3 HosekWilkie(float cosTheta, float gamma, float cosGamma)
{
    float3 A = weatherData.A;
    float3 B = weatherData.B;
    float3 C = weatherData.C;
    float3 D = weatherData.D;
    float3 E = weatherData.E;
    float3 F = weatherData.F;
    float3 G = weatherData.G;
    float3 H = weatherData.H;
    float3 I = weatherData.I;
    float3 Z = weatherData.Z;

    float3 chi = (1 + cosGamma * cosGamma) / pow(1 + H * H - 2 * cosGamma * H, float3(1.5f, 1.5f, 1.5f));
    return (1 + A * exp(B / (cosTheta + 0.01))) * (C + D * exp(E * gamma) + F * (cosGamma * cosGamma) + G * chi + I * sqrt(cosTheta));
}

float3 HosekWilkieSkyColor(float3 v, float3 sunDir)
{
    float cosTheta = clamp(v.y, 0, 1);
    float cosGamma = clamp(dot(v, sunDir), 0, 1);
    float gamma = acos(cosGamma);
    float3 R = -weatherData.Z * HosekWilkie(cosTheta, gamma, cosGamma);
    return R;
}

float4 HosekWilkieSky(VSToPS input) : SV_TARGET
{
    float3 dir = normalize(input.Dir);
    float3 col = HosekWilkieSkyColor(dir, weatherData.lightDir.xyz);
    return float4(col, 1.0);
}