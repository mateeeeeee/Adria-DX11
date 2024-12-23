#include <Common.hlsli>

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

Texture2D LdrSceneTx : register(t0);

float4 FXAA(VSToPS input) : SV_TARGET
{
    float2 texCoord = input.Tex;
    uint width, height, levels;
    LdrSceneTx.GetDimensions(0, width, height, levels); 
    float2 resolution = float2(width, height);

    const float FXAA_SPAN_MAX = 8.0;
    const float FXAA_REDUCE_MUL = 1.0 / 8.0;
    const float FXAA_REDUCE_MIN = 1.0 / 128.0;
    
    float3 rgbNW = LdrSceneTx.Sample(LinearWrapSampler, texCoord + float2(-1.0, -1.0) / resolution, 0).rgb;
    float3 rgbNE = LdrSceneTx.Sample(LinearWrapSampler, texCoord + float2(1.0, -1.0) / resolution, 0).rgb;
    float3 rgbSW = LdrSceneTx.Sample(LinearWrapSampler, texCoord + float2(-1.0, 1.0) / resolution, 0).rgb;
    float3 rgbSE = LdrSceneTx.Sample(LinearWrapSampler, texCoord + float2(1.0, 1.0) / resolution, 0).rgb;
    float3 rgbM  = LdrSceneTx.Sample(LinearWrapSampler, texCoord, 0).rgb;
    
    float3 luma = float3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM, luma);
    
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);
    
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    dir = min(float2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
          max(float2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
          dir * rcpDirMin)) / resolution;
    
    float3 rgbA = (1.0 / 2.0) *
    (
        LdrSceneTx.Sample(LinearWrapSampler, texCoord + dir * (1.0 / 3.0 - 0.5), 0).rgb +
        LdrSceneTx.Sample(LinearWrapSampler, texCoord + dir * (2.0 / 3.0 - 0.5), 0).rgb
    );
    
    float3 rgbB = rgbA * (1.0 / 2.0) + (1.0 / 4.0) *
    (
       LdrSceneTx.Sample(LinearWrapSampler, texCoord + dir * (0.0 / 3.0 - 0.5), 0).rgb +
       LdrSceneTx.Sample(LinearWrapSampler, texCoord + dir * (3.0 / 3.0 - 0.5), 0).rgb
    );
    
    float lumaB = dot(rgbB, luma);

    float3 finalColor;
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        finalColor.xyz = rgbA;
    }
    else
    {
        finalColor.xyz = rgbB;
    }
    return float4(finalColor, 1.0f);
	
}