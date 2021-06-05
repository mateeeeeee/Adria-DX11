#include "../Util/LightUtil.hlsli"
#include "../Util/VoxelUtil.hlsli"



static const int SSAO_KERNEL_SIZE = 16;


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
}

cbuffer LightCbuf : register(b2)
{
    Light current_light;
}

cbuffer ShadowCBuf : register(b3)
{
    row_major matrix lightviewprojection;
    row_major matrix lightview;
    row_major matrix shadow_matrix1;
    row_major matrix shadow_matrix2;
    row_major matrix shadow_matrix3; 
    float3 splits;
    float softness;
    int shadow_map_size;
    int visualize;
}

cbuffer MaterialCBuf : register(b4)
{
    float3 ambient;
    float3 diffuse;
    float3 specular;
    float shininess;
    
    float albedo_factor;
    float metallic_factor;
    float roughness_factor;
    float emissive_factor;
};


cbuffer PostprocessCBuf : register(b5)
{
    float2  ssao_noise_scale;
    float   ssao_radius;
    float   ssao_power;
    float4  ssao_samples[SSAO_KERNEL_SIZE];
    float   ssr_ray_step;
    float   ssr_ray_hit_threshold;
    float   motion_blur_intensity;
    float   tone_map_exposure;
    float4  dof_params;
    float   fog_near;
    float   fog_far;
    float   fog_density;
    float   fog_height;
    float4  fog_color;
};

cbuffer WeatherCBuf : register(b7)
{
    float4 light_dir;
    float4 light_color;
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
}

cbuffer VoxelCbuf : register(b8)
{
    VoxelRadiance voxel_radiance;
}



SamplerState linear_wrap_sampler    : register(s0);
SamplerState point_wrap_sampler     : register(s1);
SamplerState linear_border_sampler   : register(s2);
SamplerState linear_clamp_sampler   : register(s3);
SamplerState point_clamp_sampler    : register(s4);
SamplerComparisonState shadow_sampler : register(s5);
SamplerState anisotropic_sampler    : register(s6);

float4 GetClipSpacePosition(float2 texcoord, float depth)
{
    float4 clipSpaceLocation;
    clipSpaceLocation.xy = texcoord * 2.0f - 1.0f;
    clipSpaceLocation.y *= -1;
    clipSpaceLocation.z = depth;
    clipSpaceLocation.w = 1.0f;
    
    return clipSpaceLocation;
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

float ConvertZToLinearDepth(float depth)
{
    float near = camera_near;
    float far = camera_far;
    
    return (near * far) / (far - depth * (far - near));

}

float LinearFog(float dist)
{
    return saturate((dist - fog_near) / (fog_far - fog_near));
}


float ExponentialFog(float4 pos_vs)
{
        
    float4 pos_ws = mul(pos_vs, inverse_view);
    pos_ws /= pos_ws.w;
    float3 obj_to_camera = camera_position - pos_ws;
    
    float t;
    if (pos_ws.y < fog_height)
    {
        if (camera_position.y > fog_height)
        {
            t = (fog_height - pos_ws.y) / obj_to_camera.y;
        }
        else
        {
            t = 1.0;
        }
    }
    else
    {
        if (camera_position.y < fog_height)
        {
            t = (camera_position.y - fog_height) / obj_to_camera.y;
        }
        else
        {
            t = 0.0;
        }
    }

    float distance = length(obj_to_camera) * t;
    float fog = exp(-distance * fog_density);
    return saturate(1 - fog);
}



bool IsSaturated(float value)
{
    return value == saturate(value);
}
bool IsSaturated(float2 value)
{
    return IsSaturated(value.x) && IsSaturated(value.y);
}
bool IsSaturated(float3 value)
{
    return IsSaturated(value.x) && IsSaturated(value.y) && IsSaturated(value.z);
}
bool IsSaturated(float4 value)
{
    return IsSaturated(value.x) && IsSaturated(value.y) && IsSaturated(value.z) && IsSaturated(value.w);
}
float ScreenFade(float2 uv)
{
    float2 fade = max(12.0f * abs(uv - 0.5f) - 5.0f, 0.0f);
    return saturate(1.0 - dot(fade, fade));
}
