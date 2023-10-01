#include <Common.hlsli>
Texture2D<float4> DepthTx : register(t0);


struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};


float2 MotionVectors(VSToPS input) : SV_Target0
{
    float uv = input.Tex;
    float2 currentClip = uv * float2(2, -2) + float2(-1, 1);
    float depth = DepthTx.Sample(LinearWrapSampler, uv);

	matrix reprojection = mul(frameData.inverseViewProjection, frameData.prevViewProjection);
    float4 previousClip = mul(float4(currentClip, depth, 1.0f), reprojection);
	previousClip.xy /= previousClip.w;
    
	return (previousClip.xy - currentClip) * float2(0.5f, -0.5f);
}
