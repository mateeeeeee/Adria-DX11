#include <Common.hlsli>

Texture2D SceneTx : register(t0);
Texture2D BlurredTx : register(t1);
Texture2D DepthTx : register(t2);

float BlurFactor(float depth, float4 DOF)
{
    float f0 = 1.0f - saturate((depth - DOF.x) / max(DOF.y - DOF.x, 0.01f));
    float f1 = saturate((depth - DOF.z) / max(DOF.w - DOF.z, 0.01f));
    float blur = saturate(f0 + f1);

    return blur;
}
float BlurFactor2(float depth, float2 DOF)
{
    return saturate((depth - DOF.x) / (DOF.y - DOF.x));
}

float3 DistanceDOF(float3 colorFocus, float3 colorBlurred, float depth)
{
    float blurFactor = BlurFactor(depth, postprocessData.dofParams);
    return lerp(colorFocus, colorBlurred, blurFactor);
}

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

float4 DepthOfField(VSToPS input) : SV_TARGET
{
    float4 color = SceneTx.Sample(LinearWrapSampler, input.Tex);
    float depth = DepthTx.Sample(LinearWrapSampler, input.Tex);
    float3 colorBlurred = BlurredTx.Sample(LinearWrapSampler, input.Tex).xyz;
    depth = ConvertZToLinearDepth(depth);
    color = float4(DistanceDOF(color.xyz, colorBlurred, depth), 1.0);
    return color;
}