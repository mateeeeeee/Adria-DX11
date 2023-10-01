#include <Common.hlsli>

Texture2D SceneTx : register(t0);
Texture2D<float> DepthTx : register(t1);

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};


#define EXPONENTIAL_FOG 0
#define EXPONENTIAL_HEIGHT_FOG 1


float ExponentialFog(float dist)
{
    float fogDistance = max(dist - postprocessData.fogStart, 0.0);
    
    float fog = exp(-fogDistance * postprocessData.fogDensity);
    return 1 - fog;
}

float ExponentialFog(float4 viewSpacePosition)
{
    float4 worldSpacePosition = mul(viewSpacePosition, frameData.inverseView);
    worldSpacePosition /= worldSpacePosition.w;
    float3 worldToCamera = (frameData.cameraPosition - worldSpacePosition).xyz;

    float distance = length(worldToCamera);
    float fogDistance = max(distance - postprocessData.fogStart, 0.0);
    
    float fog = exp(-fogDistance * postprocessData.fogDensity);
    return 1 - fog;
}

float ExponentialHeightFog(float4 viewSpacePosition)
{
    float4 worldSpacePosition = mul(viewSpacePosition, frameData.inverseView);
    worldSpacePosition /= worldSpacePosition.w;
    float3 cameraToWorld = (worldSpacePosition - frameData.cameraPosition ).xyz;

    float distance = length(cameraToWorld);
	float fogDist = max(distance - postprocessData.fogStart, 0.0);

	float fogHeightDensityAtViewer = exp(-postprocessData.fogFalloff * frameData.cameraPosition.y);
    float fogDistInt = fogDist * fogHeightDensityAtViewer;

	// Height based fog intensity
    float eyeToPixelY = cameraToWorld.y * (fogDist / distance);
    float t = -postprocessData.fogFalloff * eyeToPixelY;
    const float thresholdT = 0.01;
    float fogHeightInt = abs(t) > thresholdT ? (1.0 - exp(-t)) / t : 1.0;

	float fog = exp(-postprocessData.fogDensity * fogDistInt * fogHeightInt);
    return 1 - fog;
}


float4 Fog(VSToPS input) : SV_TARGET
{
    float4 sceneColor = SceneTx.Sample(LinearWrapSampler, input.Tex);  
    float depth = DepthTx.Sample(LinearWrapSampler, input.Tex);
    float3 viewSpacePosition = GetViewSpacePosition(input.Tex, depth);

    float fog = 0.0f;
    if(postprocessData.fogType == EXPONENTIAL_FOG)
    {
        fog = ExponentialFog(float4(viewSpacePosition, 1.0f));
    }
    else if(postprocessData.fogType == EXPONENTIAL_HEIGHT_FOG)
    {
        fog = ExponentialHeightFog(float4(viewSpacePosition, 1.0f));
    }
    return lerp(sceneColor, postprocessData.fogColor, fog);
}