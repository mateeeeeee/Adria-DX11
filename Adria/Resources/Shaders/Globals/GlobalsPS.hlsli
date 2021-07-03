#include "../Util/LightUtil.hlsli"
#include "../Util/VoxelUtil.hlsli"



static const int SSAO_KERNEL_SIZE = 16;

static const int HBAO_NUM_STEPS = 4;
static const int HBAO_NUM_DIRECTIONS = 8; 


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


#define EXPONENTIAL_FOG 0
#define EXPONENTIAL_HEIGHT_FOG 1

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
    float4  fog_color;
    float   fog_falloff;
    float   fog_density;
    float   fog_start;
    int     fog_type;
    float   hbao_r2;
    float   hbao_radius_to_screen;
    float   hbao_power;
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

float ExponentialFog(float dist)
{
    float fog_dist = max(dist - fog_start, 0.0);
    
    float fog = exp(-fog_dist * fog_density);
    return 1 - fog;
}

float ExponentialFog(float4 pos_vs)
{
    float4 pos_ws = mul(pos_vs, inverse_view);
    pos_ws /= pos_ws.w;
    float3 obj_to_camera = (camera_position - pos_ws).xyz;

    float distance = length(obj_to_camera);
    
    float fog_dist = max(distance - fog_start, 0.0);
    
    float fog = exp(-fog_dist * fog_density);
    return 1 - fog;
}

float ExponentialHeightFog(float4 pos_vs)
{
    float4 pos_ws = mul(pos_vs, inverse_view);
    pos_ws /= pos_ws.w;
    float3 camera_to_world = (pos_ws - camera_position).xyz;

    float distance = length(camera_to_world);
    
	// Find the fog staring distance to pixel distance
    float fogDist = max(distance - fog_start, 0.0);

	// Distance based fog intensity
    float fogHeightDensityAtViewer = exp(-fog_falloff * camera_position.y);
    float fogDistInt = fogDist * fogHeightDensityAtViewer;

	// Height based fog intensity
    float eyeToPixelY = camera_to_world.y * (fogDist / distance);
    float t = fog_falloff * eyeToPixelY;
    const float thresholdT = 0.01;
    float fogHeightInt = abs(t) > thresholdT ?
		(1.0 - exp(-t)) / t : 1.0;

	// Combine both factors to get the final factor
    float fog = exp(-fog_density * fogDistInt * fogHeightInt);

    return 1 - fog;
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

