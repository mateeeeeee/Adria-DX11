#include "../Util/VoxelUtil.hlsli"

cbuffer PerFrame : register(b0)
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
    
    float camera_near;
    
    float camera_far;
    
    float2 screen_resolution;
}


cbuffer ComputeCBuffer : register(b6)
{
    float bloom_scale;  //bloom
    float threshold;    //bloom
    
    float gauss_coeff1; //blur coefficients
    float gauss_coeff2; //blur coefficients
    float gauss_coeff3; //blur coefficients
    float gauss_coeff4; //blur coefficients
    float gauss_coeff5; //blur coefficients
    float gauss_coeff6; //blur coefficients
    float gauss_coeff7; //blur coefficients
    float gauss_coeff8; //blur coefficients
    float gauss_coeff9; //blur coefficients
    
    float bokeh_fallout;        //bokeh
    float4 dof_params;          //bokeh
    float bokeh_radius_scale;   //bokeh
    float bokeh_color_scale;    //bokeh
    float bokeh_blur_threshold; //bokeh
    float bokeh_lum_threshold;  //bokeh	
    
    int ocean_size;             //ocean
    int resolution;             //ocean
    float ocean_choppiness;     //ocean								
    float wind_direction_x;     //ocean
    float wind_direction_y;     //ocean
    float delta_time;           //ocean
    int visualize_tiled;        //tiled rendering
    int lights_count_white;
}


cbuffer VoxelCbuf : register(b8)
{
    VoxelRadiance voxel_radiance;
}



SamplerState linear_wrap_sampler    : register(s0);
SamplerState point_wrap_sampler     : register(s1);
SamplerState linear_border_sampler  : register(s2);
SamplerState linear_clamp_sampler   : register(s3);


float ConvertZToLinearDepth(float depth)
{
    float near = camera_near;
    float far = camera_far;
    
    return (near * far) / (far - depth * (far - near));

}

float3 GetPositionVS(float2 texcoord, float depth)
{
    float4 clipSpaceLocation;
    clipSpaceLocation.xy = texcoord * 2.0f - 1.0f;
    clipSpaceLocation.y *= -1;
    clipSpaceLocation.z = depth;
    clipSpaceLocation.w = 1.0f;
    float4 homogenousLocation = mul(clipSpaceLocation, inverse_projection);
    return homogenousLocation.xyz / homogenousLocation.w;
}