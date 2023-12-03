#include <Common.hlsli>
#include <Constants.hlsli>

Texture2D<float4> InputTx       : register(t0);

float2 ApplyLensDistortion(float2 uv)
{
	if (postprocessData.lensDistortionEnabled)
	{
		const float2 center = float2(0.5, 0.5);
		float2 distortionVector = uv - center;
		float  distortionRadius = length(distortionVector);
		float  distortionFactor = 1.0 + postprocessData.lensDistortionIntensity * distortionRadius * distortionRadius;
		uv = center + distortionVector * distortionFactor;
	}
	return uv;
}

float3 SampleWithChromaticAberration(float2 uv)
{

	float3 color = 0.0f;
	if (postprocessData.chromaticAberrationEnabled)
	{
		float2 distortion = (uv - 0.5f) * postprocessData.chromaticAberrationIntensity / frameData.screenResolution;

		float2 uv_R = uv + float2(1, 1) * distortion;
		float2 uv_G = uv + float2(0, 0) * distortion;
		float2 uv_B = uv - float2(1, 1) * distortion;

		float R = InputTx.SampleLevel(LinearBorderSampler, uv_R, 0).r;
		float G = InputTx.SampleLevel(LinearBorderSampler, uv_G, 0).g;
		float B = InputTx.SampleLevel(LinearBorderSampler, uv_B, 0).b;
		color = float3(R, G, B);
	}
	else
	{
		color = InputTx.SampleLevel(LinearBorderSampler, uv, 0).rgb;
	}
	return color;
}

float3 ApplyVignette(float3 color, float2 uv)
{
	if (postprocessData.vignetteEnabled)
	{
		const float2 uvCenter = float2(0.5f, 0.5f);
		float2 uvFromCenter = abs(uv - uvCenter) / float2(uvCenter);

		float2 vignetteMask = cos(uvFromCenter * postprocessData.vignetteIntensity * M_PI_DIV_4);
		vignetteMask = vignetteMask * vignetteMask;
		vignetteMask = vignetteMask * vignetteMask;
		color *= clamp(vignetteMask.x * vignetteMask.y, 0, 1);
	}
	return color;
}

uint3 PCG3D16(uint3 v)
{
    v = v * 12829u + 47989u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v >>= 16u;
    return v;
}

float2 Simplex(float2 P)
{
    const float F2 = (sqrt(3.0) - 1.0) / 2.0; 
    const float G2 = (3.0 - sqrt(3.0)) / 6.0; 
    float   u   = (P.x + P.y) * F2;
    float2 Pi   = round(P + u);
    float  v    = (Pi.x + Pi.y) * G2;
    float2 P0   = Pi - v;  
    return P - P0;  
}

float3 ApplyFilmGrain(float3 color, uint2 coord)
{
	if (postprocessData.filmGrainEnabled)
	{
		float  filmGrainScale	= postprocessData.filmGrainScale;
		float  filmGrainAmount	= postprocessData.filmGrainAmount;
		uint   filmGrainSeed	= postprocessData.filmGrainSeed;

		float2     randomNumberFine = PCG3D16(uint3(coord / (filmGrainScale / 8.0), filmGrainSeed)).xy;
		float2     simplex = Simplex(coord / filmGrainScale + randomNumberFine);

		const float grainShape = 3.0;
		float grain = 1 - 2 * exp2(-length(simplex) * grainShape);
		color += grain * min(color, 1 - color) * filmGrainAmount;
	}
	return color;
}

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

float4 FilmEffects(VSToPS input) : SV_TARGET
{
	float2 uv = input.Tex;
	uv = ApplyLensDistortion(uv);
	float3 color = SampleWithChromaticAberration(uv);
	color = ApplyVignette(color, uv);
	uint2 coords = uint2(uv.x * frameData.screenResolution.x - 0.5f, (1 - uv.y) * frameData.screenResolution.y - 0.5f);
	color = ApplyFilmGrain(color, coords);
	return float4(color, 1.0f);
}