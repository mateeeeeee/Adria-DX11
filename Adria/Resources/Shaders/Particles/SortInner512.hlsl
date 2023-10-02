#define SORT_SIZE       512
#define NUM_THREADS		(SORT_SIZE/2)
#define INVERSION		(16*2 + 8*3)


cbuffer NumElementsCB : register(b11)
{
    int4 NumElements;
};

RWStructuredBuffer<float2> Data : register(u0);
groupshared float2 LDS[SORT_SIZE];


[numthreads(NUM_THREADS, 1, 1)]
void SortInner512CS(uint3 groupId : SV_GroupID,
		  uint3 dispatchThreadId : SV_DispatchThreadID,
		  uint3 groupThreadId : SV_GroupThreadID,
		  uint groupIndex : SV_GroupIndex)
{
    int4 tgp;
    tgp.x = groupId.x * 256;
    tgp.y = 0;
    tgp.z = NumElements.x;
    tgp.w = min(512, max(0, NumElements.x - groupId.x * 512));

    int globalBaseIndex = tgp.y + tgp.x * 2 + groupThreadId.x;
    int localBaseIndex = groupIndex;
    int i;

	[unroll]
    for (i = 0; i < 2; ++i)
    {
        if (groupIndex + i * NUM_THREADS < tgp.w)
            LDS[localBaseIndex + i * NUM_THREADS] = Data[globalBaseIndex + i * NUM_THREADS];
    }
    GroupMemoryBarrierWithGroupSync();

	for (int mergeSubSize = SORT_SIZE >> 1; mergeSubSize > 0; mergeSubSize = mergeSubSize >> 1)
    {
        int tmpIndex = groupIndex;
        int lowIndex = tmpIndex & (mergeSubSize - 1);
        int highIndex = 2 * (tmpIndex - lowIndex);
        int index = highIndex + lowIndex;
        uint swapElem = highIndex + mergeSubSize + lowIndex;

        if (swapElem < tgp.w)
        {
            float2 a = LDS[index];
            float2 b = LDS[swapElem];

            if (a.x > b.x)
            {
                LDS[index] = b;
                LDS[swapElem] = a;
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }
    
	[unroll]
    for (i = 0; i < 2; ++i)
    {
        if (groupIndex + i * NUM_THREADS < tgp.w)
            Data[globalBaseIndex + i * NUM_THREADS] = LDS[localBaseIndex + i * NUM_THREADS];
    }
}
