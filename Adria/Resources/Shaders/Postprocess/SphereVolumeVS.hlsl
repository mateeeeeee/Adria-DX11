#include "../Util/LightVolumesUtil.hlsli"
#include "../Globals/GlobalsVS.hlsli"

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEX;
};

VertexOut main(uint vid : SV_VERTEXID) 
{
    VertexOut vout;
    
    float4 pos = ICOSPHERE[vid];
    float4 world_pos = mul(pos, model);
    vout.PosH = mul(world_pos, viewprojection);
    vout.PosH /= vout.PosH.w;
    vout.Tex = vout.PosH.xy / vout.PosH.w * float2(0.5f, -0.5f) + 0.5f;

    return vout;

}