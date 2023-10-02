#define SORT_SIZE       512
#define ITEMS_PER_GROUP	( SORT_SIZE )
#define HALF_SIZE		(SORT_SIZE/2)
#define ITERATIONS		(HALF_SIZE > 1024 ? HALF_SIZE/1024 : 1)
#define NUM_THREADS		(HALF_SIZE/ITERATIONS)
#define INVERSION		(16*2 + 8*3)



RWStructuredBuffer<float2> Data : register(u0);
groupshared float2 LDS[SORT_SIZE];

cbuffer NumElementsCBuffer : register(b11)
{
    int4 NumElements;
};

[numthreads(NUM_THREADS, 1, 1)]
void Sort512CS(uint3 groupId : SV_GroupID,
		  uint3 dispatchThreadId : SV_DispatchThreadID,
		  uint3 groupThreadId : SV_GroupThreadID,
		  uint groupIndex : SV_GroupIndex)
{
    int globalBaseIndex = (groupId.x * SORT_SIZE) + groupThreadId.x;
    int localBaseIndex = groupIndex;
    int numElementsInThreadGroup = min(SORT_SIZE, NumElements.x - (groupId.x * SORT_SIZE));
	

    int i;
	[unroll]
    for (i = 0; i < 2 * ITERATIONS; ++i)
    {
        if (groupIndex + i * NUM_THREADS < numElementsInThreadGroup)
            LDS[localBaseIndex + i * NUM_THREADS] = Data[globalBaseIndex + i * NUM_THREADS];
    }
    GroupMemoryBarrierWithGroupSync();
    
	for (unsigned int mergeSize = 2; mergeSize <= SORT_SIZE; mergeSize = mergeSize * 2)
    {
        for (int mergeSubSize = mergeSize >> 1; mergeSubSize > 0; mergeSubSize = mergeSubSize >> 1)
        {
			[unroll]
            for (i = 0; i < ITERATIONS; ++i)
            {
                int tmpIndex = groupIndex + NUM_THREADS * i;
                int lowIndex = tmpIndex & (mergeSubSize - 1);
                int highIndex = 2 * (tmpIndex - lowIndex);
                int index = highIndex + lowIndex;

                uint swapElem = mergeSubSize == mergeSize >> 1 ? highIndex + (2 * mergeSubSize - 1) - lowIndex : highIndex + mergeSubSize + lowIndex;
                if (swapElem < numElementsInThreadGroup)
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
        }
    }
    
	[unroll]
    for (i = 0; i < 2 * ITERATIONS; ++i)
    {
        if (groupIndex + i * NUM_THREADS < numElementsInThreadGroup)
            Data[globalBaseIndex + i * NUM_THREADS] = LDS[localBaseIndex + i * NUM_THREADS];
    }
}