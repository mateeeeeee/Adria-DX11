#include <Constants.hlsli>

#define FFT_WORK_GROUP_DIM 256

Texture2D<float4>   InputTx   : register(t0);
RWTexture2D<float4> OutputTx  : register(u0);


cbuffer FFTCBuffer : register(b10)
{
    int SeqCount;
    int SubseqCount;
}

float2 MultiplyComplex(float2 a, float2 b)
{
    return float2(a[0] * b[0] - a[1] * b[1], a[1] * b[0] + a[0] * b[1]);
}

float4 ButterflyOperation(float2 a, float2 b, float2 twiddle)
{
    float2 twiddleB = MultiplyComplex(twiddle, b);
    float4 result = float4(a + twiddleB, a - twiddleB);
    return result;
}


[numthreads(FFT_WORK_GROUP_DIM, 1, 1)]
void FFT_HorizontalCS(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)
{
    uint2 pixelCoord = uint2(groupThreadId.x, groupId.x);

    int threadCount = int(SeqCount * 0.5f);
    int threadIdx = pixelCoord.x;

    int inIdx = threadIdx & (SubseqCount - 1);
    int outIdx = ((threadIdx - inIdx) << 1) + inIdx;

    float angle = -M_PI * (float(inIdx) / float(SubseqCount));
    float2 twiddle = float2(cos(angle), sin(angle));
    
    float4 a = InputTx.Load(uint3(pixelCoord, 0));
    float4 b = InputTx.Load(uint3(pixelCoord.x + threadCount, pixelCoord.y, 0));

    float4 result0 = ButterflyOperation(a.xy, b.xy, twiddle);
    float4 result1 = ButterflyOperation(a.zw, b.zw, twiddle);
    
    OutputTx[int2(outIdx, pixelCoord.y)] = float4(result0.xy, result1.xy);
    OutputTx[int2(outIdx + SubseqCount, pixelCoord.y)] = float4(result0.zw, result1.zw);
}