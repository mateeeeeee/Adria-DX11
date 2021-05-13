cbuffer PerFrame : register(b0)
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
    
    float fog_near;
    
    float fog_far;
}

SamplerState linear_wrap_sampler : register(s0);