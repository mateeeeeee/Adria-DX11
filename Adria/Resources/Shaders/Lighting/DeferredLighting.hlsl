#include <Common.hlsli>
#include <Util/LightUtil.hlsli>
#include <Util/DitherUtil.hlsli>
#include <Util/ShadowUtil.hlsli>


Texture2D        NormalMetallicTx       : register(t0);
Texture2D        DiffuseRoughnessTx     : register(t1);
Texture2D<float> DepthTx                : register(t2);

Texture2D<float>       ShadowMap       : register(t4);
TextureCube<float>     ShadowCubeMap   : register(t5);
Texture2DArray<float>  ShadowCascadeMaps : register(t6);


//https://panoskarabelas.com/posts/screen_space_shadows/
static const uint  SSCS_MAX_STEPS = 16; // Max ray steps, affects quality and performance.

float SSCS(float3 viewSpacePosition)
{
    float3 rayPos = viewSpacePosition;
    float2 rayUV = 0.0f;

    float4 projectedRay = mul(float4(rayPos, 1.0f), frameData.projection);
    projectedRay.xy /= projectedRay.w;
    rayUV = projectedRay.xy * float2(0.5f, -0.5f) + 0.5f;

    float depth = DepthTx.Sample(PointClampSampler, rayUV);
    float linearDepth = ConvertZToLinearDepth(depth);
    const float SSCS_STEP_LENGTH = lightData.sscsMaxRayDistance / (float) SSCS_MAX_STEPS;

    if (linearDepth > lightData.sscsMaxDepthDistance) return 1.0f;

    float3 rayDir = normalize(-lightData.direction.xyz);
    float3 rayStep = rayDir * SSCS_STEP_LENGTH;
    
    float occlusion = 0.0f;
    [unroll(SSCS_MAX_STEPS)]
    for (uint i = 0; i < SSCS_MAX_STEPS; i++)
    {
        rayPos += rayStep;
        projectedRay = mul(float4(rayPos, 1.0), frameData.projection);
        projectedRay.xy /= projectedRay.w;
        rayUV = projectedRay.xy * float2(0.5f, -0.5f) + 0.5f;

        [branch]
        if (IsSaturated(rayUV))
        {
            depth = DepthTx.Sample(PointClampSampler, rayUV);
            linearDepth = ConvertZToLinearDepth(depth);
            float depthDelta = projectedRay.z - linearDepth;

            if (depthDelta > 0 && (depthDelta < lightData.sscsThickness))
            {
                occlusion = 1.0f;
                float2 fade = max(12 * abs(rayUV - 0.5) - 5, 0);
                occlusion *= saturate(1 - dot(fade, fade));
                break;
            }
        }
    }

    return 1.0f - occlusion;
}


struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};


float4 DeferredLightingPS(VSToPS input) : SV_TARGET
{
    float depth = DepthTx.Sample(LinearWrapSampler, input.Tex);
    float3 viewPosition = GetViewSpacePosition(input.Tex, depth);
    float3 V = normalize(0.0f.xxx - viewPosition);

	float4 normalMetallic = NormalMetallicTx.Sample(LinearWrapSampler, input.Tex);
	float3 viewNormal = 2 * normalMetallic.rgb - 1.0;
	float metallic = normalMetallic.a;

    float4 diffuseRoughness = DiffuseRoughnessTx.Sample(LinearWrapSampler, input.Tex);
    float roughness = diffuseRoughness.a;
  
    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    switch (lightData.type)
    {
        case DIRECTIONAL_LIGHT:
            Lo = DoDirectionalLightPBR(lightData, viewPosition, viewNormal, V, diffuseRoughness.rgb, metallic, roughness);
            break;
        case POINT_LIGHT:
            Lo = DoPointLightPBR(lightData, viewPosition, viewNormal, V, diffuseRoughness.rgb, metallic, roughness);
            break;
        case SPOT_LIGHT:
            Lo = DoSpotLightPBR(lightData, viewPosition, viewNormal, V, diffuseRoughness.rgb, metallic, roughness);
            break;
        default:
            return float4(1, 0, 0, 1);
    }

    if (lightData.castsShadows)
    {
        float shadowFactor = 1.0f;
        if (lightData.type == POINT_LIGHT)
        {
            const float zf = lightData.range;
            const float zn = 0.5f;
            const float c1 = zf / (zf - zn);
            const float c0 = -zn * zf / (zf - zn);
            
            float3 lightToPixel = mul(float4(viewPosition - lightData.position.xyz, 0.0f), frameData.inverseView);
            const float3 m = abs(lightToPixel).xyz;
            const float major = max(m.x, max(m.y, m.z));

            float fragmentDepth = (c1 * major + c0) / major;
            shadowFactor = ShadowCubeMap.SampleCmpLevelZero(ShadowSampler, normalize(lightToPixel.xyz), fragmentDepth);
        }
        else if (lightData.type == DIRECTIONAL_LIGHT && lightData.useCascades)
        {
            float view_depth = viewPosition.z;
            for (uint i = 0; i < 4; ++i)
            {
                matrix lightSpaceMatrix = shadowData.shadowMatrices[i];
                if (view_depth < shadowData.splits[i])
                {
                    float4 shadowMapCoords = mul(float4(viewPosition, 1.0), lightSpaceMatrix);
                    float3 UVD = shadowMapCoords.xyz / shadowMapCoords.w;
                    UVD.xy = 0.5 * UVD.xy + 0.5;
                    UVD.y = 1.0 - UVD.y;
                    shadowFactor = CSMCalcShadowFactor_PCF3x3(ShadowSampler, ShadowCascadeMaps, i, UVD, shadowData.shadowMapSize, shadowData.softness);
                    break;
                }
            }    
        }
        else
        {
            float4 shadowMapCoords = mul(float4(viewPosition, 1.0), shadowData.shadowMatrices[0]);
            float3 UVD = shadowMapCoords.xyz / shadowMapCoords.w;
            UVD.xy = 0.5 * UVD.xy + 0.5;
            UVD.y = 1.0 - UVD.y; 
            shadowFactor = CalcShadowFactor_PCF3x3(ShadowSampler, ShadowMap, UVD, shadowData.shadowMapSize, shadowData.softness);
        }
        Lo = Lo * shadowFactor;
    }
    
    if (lightData.screenSpaceShadows) Lo = Lo * SSCS(viewPosition);
    return float4(Lo, 1.0f);
}


