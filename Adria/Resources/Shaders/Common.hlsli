#include <CommonData.hlsli>

cbuffer FrameCBuffer  : register(b0)
{
   FrameData frameData;
}

cbuffer ObjectCBuffer : register(b1)
{
    ObjectData objectData;
}

cbuffer ShadowCBuffer : register(b3)
{
    ShadowData shadowData;
}

cbuffer MaterialCBuffer : register(b4)
{
    MaterialData materialData;
}

cbuffer PostprocessCBuffer : register(b5)
{
    PostprocessData postprocessData;
}

cbuffer ComputeCBuffer : register(b6)
{
    ComputeData computeData;
}

cbuffer WeatherCBuffer : register(b7)
{
    WeatherData weatherData;
}

SamplerState LinearWrapSampler    : register(s0);
SamplerState PointWrapSampler     : register(s1);
SamplerState LinearBorderSampler  : register(s2);
SamplerState LinearClampSampler   : register(s3);
SamplerState PointClampSampler    : register(s4);
SamplerComparisonState ShadowSampler : register(s5);
SamplerState AnisotropicSampler    : register(s6);

static float3 GetViewSpacePosition(float2 texcoord, float depth)
{
    float4 clipSpaceLocation;
    clipSpaceLocation.xy = texcoord * 2.0f - 1.0f;
    clipSpaceLocation.y *= -1;
    clipSpaceLocation.z = depth;
    clipSpaceLocation.w = 1.0f;
    float4 homogenousLocation = mul(clipSpaceLocation, frameData.inverseProjection);
    return homogenousLocation.xyz / homogenousLocation.w;
}

static float4 GetClipSpacePosition(float2 texcoord, float depth)
{
    float4 clipSpaceLocation;
    clipSpaceLocation.xy = texcoord * 2.0f - 1.0f;
    clipSpaceLocation.y *= -1;
    clipSpaceLocation.z = depth;
    clipSpaceLocation.w = 1.0f;
    
    return clipSpaceLocation;
}

static float ConvertZToLinearDepth(float depth)
{
    float cameraNear = frameData.cameraNear;
    float cameraFar  = frameData.cameraFar;
    return (cameraNear * cameraFar) / (cameraFar - depth * (cameraFar - cameraNear));
}


inline bool IsSaturated(float value)
{
    return value == saturate(value);
}
inline bool IsSaturated(float2 value)
{
    return IsSaturated(value.x) && IsSaturated(value.y);
}
inline bool IsSaturated(float3 value)
{
    return IsSaturated(value.x) && IsSaturated(value.y) && IsSaturated(value.z);
}
inline bool IsSaturated(float4 value)
{
    return IsSaturated(value.x) && IsSaturated(value.y) && IsSaturated(value.z) && IsSaturated(value.w);
}

