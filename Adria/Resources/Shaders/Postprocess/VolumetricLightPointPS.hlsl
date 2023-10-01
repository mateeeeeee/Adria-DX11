
#include "../Globals/GlobalsPS.hlsli"
#include "../Util/DitherUtil.hlsli"
#include "../Util/ShadowUtil.hlsli"
//#endif

Texture2D<float>   DepthTx        : register(t2);
TextureCube<float> ShadowCubeMap   : register(t5);

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};


float4 main(VSToPS input) : SV_TARGET
{
    float depth = max(input.Pos.z, DepthTx.SampleLevel(linear_clamp_sampler, input.Tex, 2));
    float3 P = GetPositionVS(input.Tex, depth);
    float3 V = float3(0.0f, 0.0f, 0.0f) - P;
    float cameraDistance = length(V);
    V /= cameraDistance;

    float marchedDistance = 0;
    float accumulation = 0;

    float3 rayEnd = float3(0.0f, 0.0f, 0.0f);
    const uint sampleCount = 16;
    const float stepSize = length(P - rayEnd) / sampleCount;
    P = P + V * stepSize * BayerDither(input.Pos.xy);
	[loop]
    for (uint i = 0; i < sampleCount; ++i)
    {
        float3 L = current_light.position.xyz - P; //position in view space
        const float dist2 = dot(L, L);
        const float dist = sqrt(dist2);
        L /= dist;
        float SpotFactor = dot(L, normalize(-current_light.direction.xyz));
        float spotCutOff = current_light.outerCosine;
        float attenuation = DoAttenuation(dist, current_light.range);
		[branch]
        if (current_light.castsShadows)
        {
            const float zf = current_light.range;
            const float zn = 0.5f;
            const float c1 = zf / (zf - zn);
            const float c0 = -zn * zf / (zf - zn);
            
            float3 light_to_pixelWS = mul(float4(P - current_light.position.xyz, 0.0f), inverse_view).xyz;

            const float3 m = abs(light_to_pixelWS).xyz;
            const float major = max(m.x, max(m.y, m.z));
            float fragment_depth = (c1 * major + c0) / major;
            float shadow_factor = ShadowCubeMap.SampleCmpLevelZero(shadow_sampler, normalize(light_to_pixelWS.xyz), fragment_depth);
            attenuation *= shadow_factor;
        }
        //attenuation *= ExponentialFog(cameraDistance - marchedDistance);
        accumulation += attenuation;
        marchedDistance += stepSize;
        P = P + V * stepSize;
    }
    accumulation /= sampleCount;
    return max(0, float4(accumulation * current_light.color.rgb * current_light.volumetricStrength, 1));
}