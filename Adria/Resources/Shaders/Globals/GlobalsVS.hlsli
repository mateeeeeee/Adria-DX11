#include "../Util/VoxelUtil.hlsli"

cbuffer FrameCBuf : register(b0)
{
    float4 global_ambient;
    row_major matrix view;
    row_major matrix projection;
    row_major matrix viewprojection;
    row_major matrix inverse_view;
    row_major matrix inverse_projection;
    row_major matrix inverse_view_projection;
    row_major matrix prev_view;
    row_major matrix prev_projection;
    row_major matrix prev_view_projection;
    float4 camera_position;
    float4 camera_forward;
    float camera_near;
    float camera_far;
    float2 screen_resolution;
}

cbuffer ObjectCBuf : register(b1)
{
    row_major matrix model;
    row_major matrix transposed_inverse_model;
}

cbuffer ShadowCBuf : register(b3)
{
    row_major matrix lightviewprojection;
    row_major matrix lightview;
    row_major matrix shadow_matrix1;
    row_major matrix shadow_matrix2;
    row_major matrix shadow_matrix3; //for cascades three 
    float split0;
    float split1;
    float split3;
    float softness;
    int shadow_map_size;
    int visualize;
}

cbuffer WeatherCBuf : register(b7)
{
    float4 light_dir;
    float4 light_color;
    float4 sky_color;
    float4 ambient_color;
    float4 wind_dir;
    
    float wind_speed;
    float time;
    float crispiness;
    float curliness;
    
    float coverage;
    float absorption;
    float clouds_bottom_height;
    float clouds_top_height;
    
    float density_factor;
    float cloud_type;
    
    //padd float2
    
    float3 A;
    float3 B;
    float3 C;
    float3 D;
    float3 E;
    float3 F;
    float3 G;
    float3 H;
    float3 I;
    float3 Z;
}

cbuffer VoxelCbuf : register(b8)
{
    VoxelRadiance voxel_radiance;
}

SamplerState linear_wrap_sampler : register(s0);