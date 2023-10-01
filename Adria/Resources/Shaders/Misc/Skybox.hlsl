#include <Common.hlsli>

struct VSInput
{
    float3 PosL : POSITION;
};

struct VSToPS
{
    float4 Pos : SV_POSITION;
    float3 PosL : POSITION;
};

VSToPS SkyVS(VSInput input)
{
    VSToPS output = (VSToPS)0;
    output.Pos = mul(mul(float4(input.PosL, 1.0f), objectData.model), frameData.viewprojection).xyww;
    output.PosL = input.PosL;
    return output;
}

TextureCube SkyboxTx : register(t0);

float4 SkyboxPS(VSToPS input) : SV_Target
{
    return SkyboxTx.Sample(LinearWrapSampler, input.PosL);
}