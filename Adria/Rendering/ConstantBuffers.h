#pragma once
#include "Core/CoreTypes.h"
#include <DirectXMath.h>

#ifndef DECLSPEC_ALIGN
#define DECLSPEC_ALIGN(x)   __declspec(align(x))
#endif

namespace adria
{
	DECLSPEC_ALIGN(16) struct FrameCBuffer
	{
		DirectX::XMVECTOR global_ambient;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
		DirectX::XMMATRIX viewprojection;
		DirectX::XMMATRIX inverse_view;
		DirectX::XMMATRIX inverse_projection;
		DirectX::XMMATRIX inverse_view_projection;
		DirectX::XMMATRIX previous_view;
		DirectX::XMMATRIX previous_projection;
		DirectX::XMMATRIX previous_view_projection;
		DirectX::XMVECTOR camera_position;
		DirectX::XMVECTOR camera_forward;
		float camera_near;
		float camera_far;
		float screen_resolution_x;
		float screen_resolution_y;
		float mouse_normalized_coords_x;
		float mouse_normalized_coords_y;
	};

	DECLSPEC_ALIGN(16) struct LightCBuffer
	{
		DirectX::XMVECTOR screenspace_position;
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR direction;
		DirectX::XMVECTOR color;
		float range;
		int32   type;
		float outer_cosine;
		float inner_cosine;
		int32	casts_shadows;
		int32   use_cascades;
		float volumetric_strength;
		int32   sscs;
		float sscs_thickness;
		float sscs_max_ray_distance;
		float sscs_max_depth_distance;
		float godrays_density;
		float godrays_weight;
		float godrays_decay;
		float godrays_exposure;
	};

	DECLSPEC_ALIGN(16) struct ObjectCBuffer
	{
		DirectX::XMMATRIX model;
		DirectX::XMMATRIX transposed_inverse_model;
	};

	DECLSPEC_ALIGN(16) struct MaterialCBuffer
	{
		DirectX::XMFLOAT3 ambient;
		float _padd1;
		DirectX::XMFLOAT3 diffuse;
		float alpha_cutoff;
		DirectX::XMFLOAT3 specular;

		float shininess;
		float albedo_factor;
		float metallic_factor;
		float roughness_factor;
		float emissive_factor;
	};

	DECLSPEC_ALIGN(16) struct ShadowCBuffer
	{
		DirectX::XMMATRIX lightviewprojection;
		DirectX::XMMATRIX lightview;
		DirectX::XMMATRIX shadow_matrices[4];
		float split0;
		float split1;
		float split2;
		float split3;
		float softness;
		int32 shadow_map_size;
		int32 visualize;
	};

	DECLSPEC_ALIGN(16) struct PostprocessCBuffer
	{
		DirectX::XMFLOAT2 noise_scale;
		float ssao_radius;
		float ssao_power;
		DirectX::XMVECTOR samples[16];
		float ssr_ray_step;
		float ssr_ray_hit_threshold;
		float velocity_buffer_scale;
		float tone_map_exposure;
		DirectX::XMVECTOR dof_params;
		DirectX::XMVECTOR fog_color;
		float   fog_falloff;
		float   fog_density;
		float	  fog_start;
		int32	  fog_type;
		float   hbao_r2;
		float   hbao_radius_to_screen;
		float   hbao_power;
	};

	DECLSPEC_ALIGN(16) struct ComputeCBuffer
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

		float bokeh_fallout;				//bokeh
		DirectX::XMVECTOR dof_params;	//bokeh
		float bokeh_radius_scale;			//bokeh
		float bokeh_color_scale;			//bokeh
		float bokeh_blur_threshold;		//bokeh
		float bokeh_lum_threshold;		//bokeh	

		int32 ocean_size;					//ocean
		int32 resolution;					//ocean
		float ocean_choppiness;			//ocean								
		float wind_direction_x;			//ocean
		float wind_direction_y;			//ocean
		float delta_time;					//ocean
		int32 visualize_tiled;
		int32 visualize_max_lights;
	};

	DECLSPEC_ALIGN(16) struct WeatherCBuffer
	{
		DirectX::XMVECTOR light_dir;
		DirectX::XMVECTOR light_color;
		DirectX::XMVECTOR sky_color;
		DirectX::XMVECTOR ambient_color;
		DirectX::XMVECTOR wind_dir;

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
		float _padd[2];

		//sky parameters
		DirectX::XMFLOAT3 A; 
		float _paddA;
		DirectX::XMFLOAT3 B;
		float _paddB;
		DirectX::XMFLOAT3 C;
		float _paddC;
		DirectX::XMFLOAT3 D;
		float _paddD;
		DirectX::XMFLOAT3 E;
		float _paddE;
		DirectX::XMFLOAT3 F;
		float _paddF;
		DirectX::XMFLOAT3 G;
		float _paddG;
		DirectX::XMFLOAT3 H;
		float _paddH;
		DirectX::XMFLOAT3 I;
		float _paddI;
		DirectX::XMFLOAT3 Z;
		float _paddZ;
	};

	DECLSPEC_ALIGN(16) struct VoxelCBuffer
	{
		DirectX::XMFLOAT3 grid_center;
		float   data_size;        // voxel half-extent in world space units
		float   data_size_rcp;    // 1.0 / voxel-half extent
		uint32    data_res;         // voxel grid resolution
		float   data_res_rcp;     // 1.0 / voxel grid resolution
		uint32    num_cones;
		float   num_cones_rcp;
		float   max_distance;
		float   ray_step_size;
		uint32    mips;
	};

	DECLSPEC_ALIGN(16) struct TerrainCBuffer
	{
		DirectX::XMFLOAT2 texture_scale;
		int ocean_active;
	};

	//Structured Buffers
	struct VoxelType
	{
		uint32 color_mask;
		uint32 normal_mask;
	};
	struct LightSBuffer
	{
		DirectX::XMVECTOR position;
		DirectX::XMVECTOR direction;
		DirectX::XMVECTOR color;
		int32 active;
		float range;
		int32 type;
		float outer_cosine;
		float inner_cosine;
		int32 casts_shadows;
		int32 use_cascades;
		int32 padd;
	};
	struct ClusterAABB
	{
		DirectX::XMVECTOR min_point;
		DirectX::XMVECTOR max_point;
	};
	struct LightGrid
	{
		uint32 offset;
		uint32 light_count;
	};
}