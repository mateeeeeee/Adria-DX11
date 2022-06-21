
#include "../Globals/GlobalsPS.hlsli"
#include "../Util/DitherUtil.hlsli"
#include "../Util/ShadowUtil.hlsli"
//#endif

Texture2D<float> depthTx        : register(t2);
TextureCube depthCubeMap        : register(t5);

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEX;
};




float4 main(VertexOut input) : SV_TARGET
{
    
     //float2 ScreenCoord = input.pos2D.xy / input.pos2D.w * float2(0.5f, -0.5f) + 0.5f;
    float depth = max(input.PosH.z, depthTx.SampleLevel(linear_clamp_sampler, input.Tex, 2));
    float3 P = GetPositionVS(input.Tex, depth);
    float3 V = float3(0.0f, 0.0f, 0.0f) - P;
    float cameraDistance = length(V);
    V /= cameraDistance;

    float marchedDistance = 0;
    float accumulation = 0;

    float3 rayEnd = float3(0.0f, 0.0f, 0.0f);
	// todo: rayEnd should be clamped to the closest cone intersection point when camera is outside volume
	
    const uint sampleCount = 16;
    const float stepSize = length(P - rayEnd) / sampleCount;

	// dither ray start to help with undersampling:
    P = P + V * stepSize * dither(input.PosH.xy);

	// Perform ray marching to integrate light volume along view ray:
	[loop]
    for (uint i = 0; i < sampleCount; ++i)
    {
        float3 L = current_light.position.xyz - P; //position in view space
        const float dist2 = dot(L, L);
        const float dist = sqrt(dist2);
        L /= dist;

        float SpotFactor = dot(L, normalize(-current_light.direction.xyz));
        float spotCutOff = current_light.outer_cosine;

        float attenuation = DoAttenuation(dist, current_light.range);

		[branch]
        if (current_light.casts_shadows)
        {
            const float zf = current_light.range;
            const float zn = 0.5f;
            const float c1 = zf / (zf - zn);
            const float c0 = -zn * zf / (zf - zn);
            
            float3 light_to_pixelWS = mul(float4(P - current_light.position.xyz, 0.0f), inverse_view).xyz;

            const float3 m = abs(light_to_pixelWS).xyz;
            const float major = max(m.x, max(m.y, m.z));
            float fragment_depth = (c1 * major + c0) / major;
            float shadow_factor = depthCubeMap.SampleCmpLevelZero(shadow_sampler, normalize(light_to_pixelWS.xyz), fragment_depth);
            attenuation *= shadow_factor;
        }
        //attenuation *= ExponentialFog(cameraDistance - marchedDistance);
        accumulation += attenuation;
        marchedDistance += stepSize;
        P = P + V * stepSize;
    }
    accumulation /= sampleCount;
    return max(0, float4(accumulation * current_light.color.rgb * current_light.volumetric_strength, 1));
}