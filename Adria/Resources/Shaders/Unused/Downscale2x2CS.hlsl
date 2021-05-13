Texture2D<float4> inputTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);



[numthreads(8, 8, 1)]
void main(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID, uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_DispatchThreadID)
{
    
    uint2 pixel = uint2(dispatchID.x, dispatchID.y);

    uint2 inPixel = pixel * 2;
    float4 hIntensity0 = lerp(inputTexture[inPixel], inputTexture[inPixel + uint2(1, 0)], 0.5);
    float4 hIntensity1 = lerp(inputTexture[inPixel + uint2(0, 1)], inputTexture[inPixel + uint2(1, 1)], 0.5);
    float4 intensity   = lerp(hIntensity0, hIntensity1, 0.5);

    
    outputTexture[pixel] = float4(intensity.rgb, 1.0);
}