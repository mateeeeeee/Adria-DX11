#include <Common.hlsli>

static const float4 LUM_FACTOR = float4(0.299, 0.587, 0.114, 0);

Texture2D<float4> HDRTx   : register(t0);
Texture2D<float>  DepthTx : register(t1);

struct Bokeh
{
    float3 position;
    float2 size;
    float3 color;
};
AppendStructuredBuffer<Bokeh> BokehStack : register(u0);

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

[numthreads(32, 32, 1)]
void BokehGeneration(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 currentPixel = dispatchThreadId.xy;

    uint width, height, levels;
    HDRTx.GetDimensions(0, width, height, levels);
    
    float2 uv = float2(currentPixel.x, currentPixel.y) / float2(width - 1, height - 1);
    float depth = DepthTx.Load(int3(currentPixel, 0));
    float centerDepth = ConvertZToLinearDepth(depth);
    
    if (depth < 1.0f) 
    {
        float centerBlur = BlurFactor(centerDepth, computeData.dofParams);
        static const uint NumSamples = 9;
        const uint2 samplePoints[NumSamples] =
        {
            uint2(-1, -1), uint2(0, -1),  uint2(1, -1),
		    uint2(-1,  0), uint2(0,  0),  uint2(1,  0),
		    uint2(-1,  1), uint2(0,  1),  uint2(1,  1)
        };
        float3 centerColor = HDRTx.Load(int3(currentPixel, 0)).rgb;
        
        float3 averageColor = 0.0f;
        for (uint i = 0; i < NumSamples; ++i)
        {
            float3 sample = HDRTx.Load(int3(currentPixel + samplePoints[i], 0)).rgb;
            averageColor += sample;
        }
        averageColor /= NumSamples;

	    float averageBrightness = dot(averageColor, 1.0f);
        float centerBrightness  = dot(centerColor, 1.0f);
        float brightnessDiff    = max(centerBrightness - averageBrightness, 0.0f);

        [branch]
        if (brightnessDiff >= computeData.bokehLumThreshold && centerBlur > computeData.bokehBlurThreshold)
        {
            Bokeh bokeh;
            bokeh.position = float3(uv, centerDepth);
            bokeh.size = centerBlur * computeData.bokehRadiusScale / float2(width, height);
         
            float cocRadius = centerBlur * computeData.bokehRadiusScale * 0.45f;
            float cocArea = cocRadius * cocRadius * 3.14159f;
            float falloff = pow(saturate(1.0f / cocArea), computeData.bokehFallout);
            bokeh.color = centerColor * falloff;

            BokehStack.Append(bokeh);
        }


    }
    
}
