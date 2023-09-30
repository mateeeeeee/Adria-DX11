#include <Common.hlsli>

Texture2D<float4> InputTx : register(t0);
Texture2D<float4> BloomTx : register(t1);
RWTexture2D<float4> OutputTx : register(u0);

struct CSInput
{
	uint3 GroupId : SV_GroupID;
	uint3 GroupThreadId : SV_GroupThreadID;
	uint3 DispatchThreadId : SV_DispatchThreadID;
	uint GroupIndex : SV_GroupIndex;
};

[numthreads(32, 32, 1)]
void BloomCombine(CSInput input)
{
    uint3 dims;
    BloomTx.GetDimensions(0, dims.x, dims.y, dims.z);

    uint3 dispatchID = input.DispatchThreadId;
    float3 bloom = BloomTx.SampleLevel(LinearClampSampler, dispatchID.xy * 1.0f / dims.xy, 1.5f);
    bloom += BloomTx.SampleLevel(LinearClampSampler, dispatchID.xy * 1.0f / dims.xy, 2.5f);
    bloom += BloomTx.SampleLevel(LinearClampSampler, dispatchID.xy * 1.0f / dims.xy, 3.5f);
    bloom /= 3;

    OutputTx[dispatchID.xy] = InputTx[dispatchID.xy] + float4(bloom, 0.0f);
}