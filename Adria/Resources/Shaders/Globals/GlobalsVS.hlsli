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
    row_major matrix prev_view_projection;
    float4 camera_position;
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

cbuffer VoxelCbuf : register(b8)
{
    VoxelRadiance voxel_radiance;
}

SamplerState linear_wrap_sampler : register(s0);