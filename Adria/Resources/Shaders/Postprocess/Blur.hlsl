#include <Common.hlsli>

Texture2D<float4>   InputTx        : register(t0);
RWTexture2D<float4> OutputTx       : register(u0);

 static const uint SizeX = 1024;
 static const uint SizeY = 1024;

#ifndef VERTICAL

groupshared float4 SharedHorizontalData[4 + SizeX + 4];

[numthreads(SizeX, 1, 1)]
void BlurHorizontal(uint3 GroupID : SV_GroupID, uint3 DispatchThreadID : SV_DispatchThreadID, uint3 GroupThreadID : SV_GroupThreadID, uint GroupIndex : SV_GroupIndex)
{
    const float GaussFilter[9] = { computeData.gaussCoeff1, computeData.gaussCoeff2, computeData.gaussCoeff3, computeData.gaussCoeff4, computeData.gaussCoeff5, computeData.gaussCoeff6, computeData.gaussCoeff7, computeData.gaussCoeff8, computeData.gaussCoeff9 };
    
	float4 data = InputTx.Load(DispatchThreadID);
	SharedHorizontalData[GroupThreadID.x + 4] = data;

	if (GroupIndex == 0)
    {
        
        SharedHorizontalData[0] = InputTx.Load(DispatchThreadID - int3(4, 0, 0));
        SharedHorizontalData[1] = InputTx.Load(DispatchThreadID - int3(3, 0, 0));
        SharedHorizontalData[2] = InputTx.Load(DispatchThreadID - int3(2, 0, 0));
        SharedHorizontalData[3] = InputTx.Load(DispatchThreadID - int3(1, 0, 0));
    }

	if (GroupIndex == SizeX - 1)
    {
        SharedHorizontalData[4 + SizeX + 0] = InputTx.Load(DispatchThreadID + int3(1, 0, 0));
        SharedHorizontalData[4 + SizeX + 1] = InputTx.Load(DispatchThreadID + int3(2, 0, 0));
        SharedHorizontalData[4 + SizeX + 2] = InputTx.Load(DispatchThreadID + int3(3, 0, 0));
        SharedHorizontalData[4 + SizeX + 3] = InputTx.Load(DispatchThreadID + int3(4, 0, 0));
    }
	
	GroupMemoryBarrierWithGroupSync();

	int textureLocation = GroupThreadID.x;
	float4 blurredData = float4(0.0, 0.0, 0.0, 0.0);

    for (int x = 0; x < 9; x++)
        blurredData += SharedHorizontalData[textureLocation + x] * GaussFilter[x];
    OutputTx[DispatchThreadID.xy] = blurredData;
}

#else

groupshared float4 SharedVerticalData[4 + SizeY + 4];

[numthreads(1, SizeY, 1)]
void BlurVertical(uint3 GroupID : SV_GroupID, uint3 DispatchThreadID : SV_DispatchThreadID, uint3 GroupThreadID : SV_GroupThreadID, uint GroupIndex : SV_GroupIndex)
{
    const float GaussFilter[9] = { computeData.gaussCoeff1, computeData.gaussCoeff2, computeData.gaussCoeff3, computeData.gaussCoeff4, computeData.gaussCoeff5, computeData.gaussCoeff6, computeData.gaussCoeff7, computeData.gaussCoeff8, computeData.gaussCoeff9 };
    
	float4 data = InputTx.Load(DispatchThreadID);

	SharedVerticalData[GroupThreadID.y + 4] = data;

	if (GroupIndex == 0)
    {
        SharedVerticalData[0] = InputTx.Load(DispatchThreadID - int3(0, 4, 0));
        SharedVerticalData[1] = InputTx.Load(DispatchThreadID - int3(0, 3, 0));
        SharedVerticalData[2] = InputTx.Load(DispatchThreadID - int3(0, 2, 0));
        SharedVerticalData[3] = InputTx.Load(DispatchThreadID - int3(0, 1, 0));
    }

	// Load the data to the bottom of this line for the last few pixels to use.
    if (GroupIndex == SizeY - 1)
    {
        SharedVerticalData[4 + SizeY + 0] = InputTx.Load(DispatchThreadID + int3(0, 1, 0));
        SharedVerticalData[4 + SizeY + 1] = InputTx.Load(DispatchThreadID + int3(0, 2, 0));
        SharedVerticalData[4 + SizeY + 2] = InputTx.Load(DispatchThreadID + int3(0, 3, 0));
        SharedVerticalData[4 + SizeY + 3] = InputTx.Load(DispatchThreadID + int3(0, 4, 0));
    }

	// Synchronize all threads
    GroupMemoryBarrierWithGroupSync();

    int textureLocation = GroupThreadID.y;

    float4 blurredData = float4(0.0, 0.0, 0.0, 0.0);

    for (int y = 0; y < 9; y++)
        blurredData += SharedVerticalData[textureLocation + y] * GaussFilter[y];

    OutputTx[DispatchThreadID.xy] = blurredData;
}

#endif