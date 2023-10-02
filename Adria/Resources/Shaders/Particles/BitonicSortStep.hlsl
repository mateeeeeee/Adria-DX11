
RWStructuredBuffer<float2> Data : register(u0);

cbuffer NumElementsCBuffer : register(b11)
{
    int4 NumElements;
};

cbuffer SortConstantsCBuffer : register(b12)
{
    int4 JobParams;
};

[numthreads(256, 1, 1)]
void BitonicSortStepCS(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)
{
    int4 tgp;
    tgp.x = groupId.x * 256;
    tgp.y = 0;
    tgp.z = NumElements.x;
    tgp.w = min(512, max(0, NumElements.x - groupId.x * 512));
    uint localID = tgp.x + groupThreadId.x;

    uint lowIndex = localID & (JobParams.x - 1);
    uint highIndex = 2 * (localID - lowIndex);

    uint index = tgp.y + highIndex + lowIndex;
    uint swapElem = tgp.y + highIndex + JobParams.y + JobParams.z * lowIndex;
    if (swapElem < tgp.y + tgp.z)
    {
        float2 a = Data[index];
        float2 b = Data[swapElem];

        if (a.x > b.x)
        {
            Data[index] = b;
            Data[swapElem] = a;
        }
    }
}