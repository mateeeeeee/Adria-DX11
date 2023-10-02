AppendStructuredBuffer<uint> DeadList : register(u0);

[numthreads(256, 1, 1)]
void InitDeadListCS(uint3 id : SV_DispatchThreadID)
{
	DeadList.Append(id.x);
}