#include "../Globals/GlobalsPS.hlsli"


struct PS_INPUT
{
    float4 Pos : SV_POSITION;
};




float4 main(PS_INPUT pin) : SV_Target
{
	
    return float4(diffuse, 1.0);
}