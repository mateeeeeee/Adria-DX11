#include "../Globals/GlobalsVS.hlsli"


struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 Dir : TEXCOORD0;
};

VertexOut main(uint vI : SV_VERTEXID) 
{
    int2 texcoord = int2(vI & 1, vI >> 1);
    VertexOut vout;
    
    float4 clip_pos = float4(float2(texcoord), 0.0f, 1.0f);
    float4 world_pos = mul(clip_pos, inverse_view_projection);
    
    float3 dir = world_pos.xyz;
    dir = normalize(dir);
    vout.Dir = dir;
    
    vout.PosH = float4(2 * (texcoord.x - 0.5f), -2 * (texcoord.y - 0.5f), 1.0, 1.0);
    return vout;
}