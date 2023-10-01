#include <Common.hlsli>
#include <Util/DitherUtil.hlsli>
#include <Util/ShadowUtil.hlsli>
#include <Util/LightUtil.hlsli>

Texture2D<float>   DepthTx         : register(t2);
TextureCube<float> ShadowCubeMap   : register(t5);

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

float4 VolumetricLighting_Point(VSToPS input) : SV_TARGET
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
        float attenuation = DoAttenuation(dist, lightData.range);
		[branch]
        if (lightData.castsShadows)
        {
            const float zf = lightData.range;
            const float zn = 0.5f;
            const float c1 = zf / (zf - zn);
            const float c0 = -zn * zf / (zf - zn);
            float3 lightToWorld = mul(float4(viewSpacePosition - lightData.position.xyz, 0.0f), frameData.inverseView).xyz;

            const float3 m = abs(lightToWorld).xyz;
            const float major = max(m.x, max(m.y, m.z));
            float fragmentDepth = (c1 * major + c0) / major;
            float shadowFactor = ShadowCubeMap.SampleCmpLevelZero(ShadowSampler, normalize(lightToWorld.xyz), fragmentDepth);
            attenuation *= shadowFactor;
        }
        accumulation += attenuation;
        marchedDistance += stepSize;
        viewSpacePosition = viewSpacePosition + V * stepSize;
    }
    accumulation /= SAMPLE_COUNT;
    return max(0, float4(accumulation * lightData.color.rgb * lightData.volumetricStrength, 1));
}