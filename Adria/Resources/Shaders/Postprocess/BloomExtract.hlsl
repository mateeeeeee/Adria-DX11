#include <Common.hlsli>

Texture2D<float4> InputTx : register(t0);
RWTexture2D<float4> OutputTx : register(u0);

struct CSInput
{
	uint3 GroupId : SV_GroupID;
	uint3 GroupThreadId : SV_GroupThreadID;
	uint3 DispatchThreadId : SV_DispatchThreadID;
	uint GroupIndex : SV_GroupIndex;
};

[numthreads(32,32, 1)]
void BloomExtract(CSInput input)
{
    uint3 dispatchID = input.DispatchThreadId;
    float2 uv = dispatchID.xy;
    float3 color = inputTexture[dispatchID.xy].rgb;
    //float intensity = dot(color.xyz, float3(0.2126f, 0.7152f, 0.0722f));
    color = min(color, 10.0f); 
    color = max(color - threshold, 0.0f);
    outputTexture[dispatchID.xy] = float4(bloom_scale * color, 1.0f);
}


