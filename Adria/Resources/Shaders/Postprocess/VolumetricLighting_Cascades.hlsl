#include <Common.hlsli>
#include <Util/DitherUtil.hlsli>
#include <Util/ShadowUtil.hlsli>


Texture2D<float> DepthTx                : register(t2);
Texture2DArray<float> ShadowCascadeMaps : register(t6);

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

float4 VolumetricLighting_Cascades(VSToPS input) : SV_TARGET
{
    float depth = max(input.Pos.z, DepthTx.SampleLevel(LinearClampSampler, input.Tex, 2));
    float3 viewSpacePosition = GetViewSpacePosition(input.Tex, depth);
    float3 V = float3(0.0f, 0.0f, 0.0f) - viewSpacePosition;
    float cameraDistance = length(V);
    V /= cameraDistance;
    float3 L = lightData.direction.xyz;

    const uint SAMPLE_COUNT = 16;
    float3 rayEnd = float3(0.0f, 0.0f, 0.0f);
    float stepSize = length(viewSpacePosition - rayEnd) / SAMPLE_COUNT;
    viewSpacePosition = viewSpacePosition + V * stepSize * BayerDither(input.Pos.xy);
    float viewDepth = viewSpacePosition.z;
    
    float marchedDistance = 0;
    float3 accumulation = 0;
	[loop(SAMPLE_COUNT)]
    for (uint j = 0; j < SAMPLE_COUNT; ++j)
    {
        bool valid = false;
        for (uint cascade = 0; cascade < 4; ++cascade)
        {
            matrix lightSpaceMatrix = shadowData.shadowMatrices[cascade];       
            float4 shadowMapCoords = mul(float4(viewSpacePosition, 1.0), lightSpaceMatrix);
            float3 UVD = shadowMapCoords.xyz / shadowMapCoords.w;
            UVD.xy = 0.5 * UVD.xy + 0.5;
            UVD.y = 1.0 - UVD.y;
            [branch]
            if (viewDepth < shadowData.splits[cascade])
            {
				if(IsSaturated(UVD.xy))
				{
					float attenuation = CSMCalcShadowFactor_PCF3x3(ShadowSampler, ShadowCascadeMaps, cascade, UVD, shadowData.shadowMapSize, shadowData.softness);
                    accumulation += attenuation;
				}
                marchedDistance += stepSize;
                viewSpacePosition = viewSpacePosition + V * stepSize;
                valid = true;
                break;
            }
        }
        if (!valid)  break;
    }
    accumulation /= SAMPLE_COUNT;
    return max(0, float4(accumulation * lightData.color.rgb * lightData.volumetricStrength, 1));
}
