#include <Common.hlsli>

Texture2D<float4> NormalTx      : register(t0);
Texture2D<float4> SceneTx       : register(t1);
Texture2D<float>  DepthTx        : register(t2);


static const int SSR_MAX_STEPS = 16;
static const int SSR_BINARY_SEARCH_STEPS = 16;

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};


float4 SSRBinarySearch(float3 dir, inout float3 hitCoord)
{
    float depth;
    for (int i = 0; i < SSR_BINARY_SEARCH_STEPS; i++)
    {
        float4 projectedCoord = mul(float4(hitCoord, 1.0f), frameData.projection);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

        depth = DepthTx.SampleLevel(PointClampSampler, projectedCoord.xy, 0);
        float3 viewSpacePosition = GetViewSpacePosition(projectedCoord.xy, depth);
        float depthDifference = hitCoord.z - viewSpacePosition.z;

        if (depthDifference <= 0.0f) hitCoord += dir;

        dir *= 0.5f;
        hitCoord -= dir;
    }

    float4 projectedCoord = mul(float4(hitCoord, 1.0f), frameData.projection);
    projectedCoord.xy /= projectedCoord.w;
    projectedCoord.xy = projectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    
    depth = DepthTx.SampleLevel(PointClampSampler, projectedCoord.xy, 0);
    float3 viewSpacePosition = GetViewSpacePosition(projectedCoord.xy, depth);
    float depthDifference = hitCoord.z - viewSpacePosition.z;

    return float4(projectedCoord.xy, depth, abs(depthDifference) < postprocessData.ssrRayHitThreshold ? 1.0f : 0.0f);
}

float4 SSRRayMarch(float3 dir, inout float3 hitCoord)
{
    float depth;
    for (int i = 0; i < SSR_MAX_STEPS; i++)
    {
        hitCoord += dir;
        float4 projectedCoord = mul(float4(hitCoord, 1.0f), frameData.projection);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		depth = DepthTx.SampleLevel(PointClampSampler, projectedCoord.xy, 0);
        float3 viewSpacePosition = GetViewSpacePosition(projectedCoord.xy, depth);
        float depthDifference = hitCoord.z - viewSpacePosition.z;

		[branch]
        if (depthDifference > 0.0f)
        {
            return SSRBinarySearch(dir, hitCoord);
        }

        dir *= postprocessData.ssrRayStep;
    }
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}


bool IsInsideScreen(float2 vCoord)
{
    return !(vCoord.x < 0 || vCoord.x > 1 || vCoord.y < 0 || vCoord.y > 1);
}

float4 SSR(VSToPS input) : SV_TARGET
{
    float4 normalMetallic = NormalTx.Sample(LinearBorderSampler, input.Tex);
    float4 sceneColor = SceneTx.SampleLevel(LinearClampSampler, input.Tex, 0);
    
    float metallic = normalMetallic.a;
    if (metallic < 0.01f) return sceneColor;
    
    float3 normal = normalMetallic.rgb;
    normal = 2 * normal - 1.0;
    normal = normalize(normal);

    float depth = DepthTx.Sample(LinearClampSampler, input.Tex);
    float3 viewSpacePosition = GetViewSpacePosition(input.Tex, depth);
    float3 reflectDirection = normalize(reflect(viewSpacePosition, normal));

    float3 hitPosition = viewSpacePosition;
    float4 coords = SSRRayMarch(reflectDirection, hitPosition);

    float2 coordsEdgeFactor = float2(1, 1) - pow(saturate(abs(coords.xy - float2(0.5f, 0.5f)) * 2), 8);
    float  screenEdgeFactor = saturate(min(coordsEdgeFactor.x, coordsEdgeFactor.y));
    float  reflectionIntensity = saturate( screenEdgeFactor * saturate(reflectDirection.z) * (coords.w));

    float3 reflectionColor = reflectionIntensity * SceneTx.SampleLevel(LinearClampSampler, coords.xy, 0).rgb;
    return sceneColor + metallic * max(0, float4(reflectionColor, 1.0f));
}


