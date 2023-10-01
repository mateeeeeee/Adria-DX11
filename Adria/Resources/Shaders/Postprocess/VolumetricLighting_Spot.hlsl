#include <Common.hlsli>
#include <Util/DitherUtil.hlsli>
#include <Util/ShadowUtil.hlsli>
#include <Util/LightUtil.hlsli>

Texture2D<float> DepthTx : register(t2);
Texture2D<float> ShadowMap : register(t4);

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

float4 VolumetricLighting_Spot(VSToPS input) : SV_TARGET
{
    float depth = max(input.Pos.z, DepthTx.SampleLevel(LinearClampSampler, input.Tex, 2));
    float3 viewSpacePosition = GetViewSpacePosition(input.Tex, depth);
    float3 V = float3(0.0f, 0.0f, 0.0f) - viewSpacePosition;
    float cameraDistance = length(V);
    V /= cameraDistance;

    const uint SAMPLE_COUNT = 16;
    float3 rayEnd = float3(0.0f, 0.0f, 0.0f);
    float stepSize = length(viewSpacePosition - rayEnd) / SAMPLE_COUNT;
	viewSpacePosition = viewSpacePosition + V * stepSize * BayerDither(input.Pos.xy);

    float marchedDistance = 0;
    float accumulation = 0;
	[loop(SAMPLE_COUNT)]
    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float3 L = lightData.position.xyz - viewSpacePosition; 
        float distSquared = dot(L, L);
        float dist = sqrt(distSquared);
        L /= dist;

        float spotFactor = dot(L, normalize(-lightData.direction.xyz));
        float spotCutOff = lightData.outerCosine;

		[branch]
        if (spotFactor > spotCutOff)
        {
            float attenuation = DoAttenuation(dist, lightData.range);
            float conAtt = saturate((spotFactor - lightData.outerCosine) / (lightData.innerCosine - lightData.outerCosine));
            conAtt *= conAtt;
            attenuation *= conAtt;
			[branch]
            if (lightData.castsShadows)
            {
                float4 shadowMapCoords = mul(float4(viewSpacePosition, 1.0), shadowData.shadowMatrices[0]);
                float3 UVD = shadowMapCoords.xyz / shadowMapCoords.w;

                UVD.xy = 0.5 * UVD.xy + 0.5;
                UVD.y = 1.0 - UVD.y;
                [branch]
                if (IsSaturated(UVD.xy))
                {             
                    float shadowFactor = CalcShadowFactor_PCF3x3(ShadowSampler, ShadowMap, UVD, shadowData.shadowMapSize, shadowData.softness);
                    attenuation *= shadowFactor;
                }
            }
            accumulation += attenuation;
        }
        marchedDistance += stepSize;
        viewSpacePosition = viewSpacePosition + V * stepSize;
    }
    accumulation /= SAMPLE_COUNT;
    return max(0, float4(accumulation * lightData.color.rgb * lightData.volumetricStrength, 1));
}