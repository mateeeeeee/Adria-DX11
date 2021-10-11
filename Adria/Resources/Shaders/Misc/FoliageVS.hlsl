#include "../Globals/GlobalsVS.hlsli"

struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 Uvs : TEX;
    float3 Offset : INSTANCE_OFFSET;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEX;
};

VS_OUTPUT main(VS_INPUT vin)
{
    VS_OUTPUT vout;

    matrix model_matrix = model;
    float3 position = vin.Pos;
    position += vin.Offset;
    
    float angle = atan2(camera_forward.x, camera_forward.z) * (180.0 / 3.1415926535);
    angle *= 0.0174532925f;

    // Calculate the rotation that needs to be applied to the billboard model to face the current camera position using the arc tangent function.
    //float angle = atan2(position.x - camera_position.x, position.z - camera_position.z) * (180.0 / 3.1415926535);
    //angle *= 0.0174532925f;
    matrix billboard_matrix;
    billboard_matrix[0][0] = cos(angle);
    billboard_matrix[0][1] = 0.0f;
    billboard_matrix[0][2] = -sin(angle);
    billboard_matrix[0][3] = 0.0f;
    billboard_matrix[1][0] = 0.0f;
    billboard_matrix[1][1] = 1.0f;
    billboard_matrix[1][2] = 0.0f;
    billboard_matrix[1][3] = 0.0f;
    billboard_matrix[2][0] = sin(angle);
    billboard_matrix[2][1] = 0.0f;
    billboard_matrix[2][2] = cos(angle);
    billboard_matrix[2][3] = 0.0f;
    billboard_matrix[3][0] = 0.0f;
    billboard_matrix[3][1] = 0.0f;
    billboard_matrix[3][2] = 0.0f;
    billboard_matrix[3][3] = 1.0f;
    
    model_matrix = mul(billboard_matrix, model_matrix);
    model_matrix[3].xyz += vin.Offset;
    
    if(vin.Pos.y > 5) //5 is hardcoded size of foliage, change this later, add wind params in gui
    {
        float xWindOffset = (2 * sin(time + vin.Pos.x + vin.Pos.y + vin.Pos.z) + 1) * wind_speed * wind_dir.x * 0.005f;
        float zWindOffset = (1 * sin(2 * (time + vin.Pos.x + vin.Pos.y + vin.Pos.z)) + 0.5) * wind_speed * wind_dir.z * 0.005f;
    
        model_matrix[3].xyz += float3(xWindOffset, 0, zWindOffset);
    }
    
    matrix model_view = mul(model_matrix, view);
    float4 ViewPosition = mul(float4(vin.Pos, 1.0), model_view);

    vout.Position = mul(ViewPosition, projection);
    vout.TexCoord = vin.Uvs;
    return vout;
}