#include "../Globals/GlobalsVS.hlsli"



struct VS_INPUT
{
    float3 Position : POSITION; // vertex position 
#if HAS_TEX_COORDS
    float2 Uvs      : TEX;
#endif
    float3 Normal   : NORMAL; // vertex normal
#if  HAS_NORMAL_TEXTURE
    float3 Tan      : TANGENT;
    float3 Bitan    : BITANGENT;
#endif
};



struct VS_OUTPUT
{
    float4 Position     : SV_POSITION; // vertex position 
#if HAS_TEX_COORDS
    float2 Uvs          : TEX;
#endif
    float3 NormalVS     : NORMAL0; // vertex normal
#if HAS_NORMAL_TEXTURE
    float3 TangentWS    : TANGENT;
    float3 BitangentWS  : BITANGENT;
    float3 NormalWS     : NORMAL1;
    matrix view_matrix  : MATRIX0; //For bump mapping //CHANGE THIS IF NEED FOR MORE MATRICES BESIDES VIEW MATRIX OCCURS IN PIXEL SHADER
#endif
};


VS_OUTPUT vs_main(VS_INPUT input)
{
    VS_OUTPUT Output;
    
    float4 pos =  mul(float4(input.Position, 1.0), model);
    Output.Position = mul(pos, viewprojection);
    

    // Transform the position from object space to homogeneous projection space
   
#if HAS_TEX_COORDS
    Output.Uvs = input.Uvs;
#endif
	// Transform the normal to world space
    float3 normal_ws = mul(input.Normal, (float3x3) transposed_inverse_model);
    Output.NormalVS = mul(normal_ws, (float3x3) transpose(inverse_view));
    
    
#if HAS_NORMAL_TEXTURE
    Output.TangentWS = mul(input.Tan, (float3x3) model);
    Output.BitangentWS = mul(input.Bitan, (float3x3) model);
    Output.NormalWS = normal_ws;
    Output.view_matrix = view;
#endif
    
    return Output;
}