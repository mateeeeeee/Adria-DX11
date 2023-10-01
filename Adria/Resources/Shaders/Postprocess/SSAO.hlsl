#include <Common.hlsli>

Texture2D NormalTx : register(t1);
Texture2D<float> DepthTx : register(t2);
Texture2D NoiseTx : register(t3);  

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEX;
};

float4 SSAO(VSToPS input) : SV_TARGET
{
    float3 normal = NormalTx.Sample(LinearBorderSampler, input.Tex).rgb; 
    normal = 2 * normal - 1.0;
    normal = normalize(normal);   
    float depth = DepthTx.Sample(LinearBorderSampler, input.Tex);
    
    float3 viewSpacePosition = GetViewSpacePosition(input.Tex, depth);
    float3 randomVector = normalize(2 * NoiseTx.Sample(PointWrapSampler, input.Tex * postprocessData.ssaoNoiseScale).xyz - 1); 

    float3 tangent = normalize(randomVector - normal * dot(randomVector, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    
    float occlusion = 0.0;
    [unroll(SSAO_KERNEL_SIZE)]
    for (int i = 0; i < SSAO_KERNEL_SIZE; ++i)
    {
        float3 sampleDir = mul(postprocessData.ssaoSamples[i].xyz, TBN); 
        float3 samplePos = viewSpacePosition + sampleDir * postprocessData.ssaoRadius;

        float4 offset = float4(samplePos, 1.0);
        offset = mul(offset, frameData.projection); 
        offset.xy = ((offset.xy / offset.w) * float2(1.0f, -1.0f)) * 0.5f + 0.5f; 
        float sampleDepth = DepthTx.Sample(LinearBorderSampler, offset.xy);
        sampleDepth = GetViewSpacePosition(offset.xy, sampleDepth).z;

        float rangeCheck = smoothstep(0.0, 1.0, postprocessData.ssaoRadius / abs(viewSpacePosition.z - sampleDepth));
        occlusion += rangeCheck * step(sampleDepth, samplePos.z - 0.01); 
    }
    occlusion = 1.0 - (occlusion / SSAO_KERNEL_SIZE);
    
    return pow(abs(occlusion), postprocessData.ssaoPower);
} 