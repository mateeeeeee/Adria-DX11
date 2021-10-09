#include "../Globals/GlobalsPS.hlsli"

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 Dir : TEXCOORD0;
};

float4 main(VertexOut pin) : SV_TARGET
{
    float3 dir = normalize(pin.Dir);

    float3 col = HosekWilkieSky(dir, light_dir.xyz);

    return float4(-col, 1.0);
    
}