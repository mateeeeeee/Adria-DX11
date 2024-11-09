#pragma once
#include <DirectXMath.h>

#ifndef DECLSPEC_ALIGN
#define DECLSPEC_ALIGN(x)   __declspec(align(x))
#endif

namespace adria
{
	DECLSPEC_ALIGN(16) struct FrameCBuffer
	{
		Matrix view;
		Matrix projection;
		Matrix viewprojection;
		Matrix inverse_view;
		Matrix inverse_projection;
		Matrix inverse_view_projection;
		Matrix previous_view;
		Matrix previous_projection;
		Matrix previous_view_projection;
		Vector4 global_ambient;
		Vector4 camera_position;
		Vector4 camera_forward;
		Float camera_near;
		Float camera_far;
		Float camera_jitter_x;
		Float camera_jitter_y;
		Float screen_resolution_x;
		Float screen_resolution_y;
		Float mouse_normalized_coords_x;
		Float mouse_normalized_coords_y;
	};

	DECLSPEC_ALIGN(16) struct LightCBuffer
	{
		Vector4 screenspace_position;
		Vector4 position;
		Vector4 direction;
		Vector4 color;
		Float range;
		Sint32   type;
		Float outer_cosine;
		Float inner_cosine;
		Sint32	casts_shadows;
		Sint32   use_cascades;
		Float volumetric_strength;
		Sint32   sscs;
		Float sscs_thickness;
		Float sscs_max_ray_distance;
		Float sscs_max_depth_distance;
		Float godrays_density;
		Float godrays_weight;
		Float godrays_decay;
		Float godrays_exposure;
	};

	DECLSPEC_ALIGN(16) struct ObjectCBuffer
	{
		Matrix model;
		Matrix transposed_inverse_model;
	};

	DECLSPEC_ALIGN(16) struct MaterialCBuffer
	{
		Vector3 ambient;
		Float _padd1;
		Vector3 diffuse;
		Float alpha_cutoff;
		Vector3 specular;

		Float shininess;
		Float albedo_factor;
		Float metallic_factor;
		Float roughness_factor;
		Float emissive_factor;
	};

	DECLSPEC_ALIGN(16) struct ShadowCBuffer
	{
		Matrix lightviewprojection;
		Matrix lightview;
		Matrix shadow_matrices[4];
		Float split0;
		Float split1;
		Float split2;
		Float split3;
		Float softness;
		Sint32 shadow_map_size;
		Sint32 visualize;
	};

	DECLSPEC_ALIGN(16) struct PostprocessCBuffer
	{
		Vector2 noise_scale;
		Float ssao_radius;
		Float ssao_power;
		Vector4 samples[16];
		Float ssr_ray_step;
		Float ssr_ray_hit_threshold;
		Float velocity_buffer_scale;
		Float tone_map_exposure;
		Vector4 dof_params;
		Vector4 fog_color;
		Float   fog_falloff;
		Float   fog_density;
		Float	fog_start;
		Sint32	fog_type;
		Float   hbao_r2;
		Float   hbao_radius_to_screen;
		Float   hbao_power;

		Bool32  lens_distortion_enabled;
		Float	lens_distortion_intensity;
		Bool32  chromatic_aberration_enabled;
		Float   chromatic_aberration_intensity;
		Bool32  vignette_enabled;
		Float   vignette_intensity;
		Bool32  film_grain_enabled;
		Float   film_grain_scale;
		Float   film_grain_amount;
		Uint32  film_grain_seed;
		Uint32  input_idx;
		Uint32  output_idx;
	};

	DECLSPEC_ALIGN(16) struct ComputeCBuffer
	{
		Float bloom_scale;  //bloom
		Float threshold;    //bloom

		Float gauss_coeff1; //blur coefficients
		Float gauss_coeff2; //blur coefficients
		Float gauss_coeff3; //blur coefficients
		Float gauss_coeff4; //blur coefficients
		Float gauss_coeff5; //blur coefficients
		Float gauss_coeff6; //blur coefficients
		Float gauss_coeff7; //blur coefficients
		Float gauss_coeff8; //blur coefficients
		Float gauss_coeff9; //blur coefficients

		Float bokeh_fallout;				//bokeh
		Vector4 dof_params;					//bokeh
		Float bokeh_radius_scale;			//bokeh
		Float bokeh_color_scale;			//bokeh
		Float bokeh_blur_threshold;			//bokeh
		Float bokeh_lum_threshold;			//bokeh	

		Sint32 ocean_size;					//ocean
		Sint32 resolution;					//ocean
		Float ocean_choppiness;			//ocean								
		Float wind_direction_x;			//ocean
		Float wind_direction_y;			//ocean
		Float delta_time;					//ocean
		Sint32 visualize_tiled;
		Sint32 visualize_max_lights;
	};

	DECLSPEC_ALIGN(16) struct WeatherCBuffer
	{
		Vector4 light_dir;
		Vector4 light_color;
		Vector4 sky_color;
		Vector4 ambient_color;
		Vector4 wind_dir;

		Float wind_speed;
		Float time;
		Float crispiness;
		Float curliness;

		Float coverage;
		Float absorption;
		Float clouds_bottom_height;
		Float clouds_top_height;

		Float density_factor;
		Float cloud_type;
		Float _padd[2];

		//sky parameters
		Vector3 A; 
		Float _paddA;
		Vector3 B;
		Float _paddB;
		Vector3 C;
		Float _paddC;
		Vector3 D;
		Float _paddD;
		Vector3 E;
		Float _paddE;
		Vector3 F;
		Float _paddF;
		Vector3 G;
		Float _paddG;
		Vector3 H;
		Float _paddH;
		Vector3 I;
		Float _paddI;
		Vector3 Z;
		Float _paddZ;
	};

	DECLSPEC_ALIGN(16) struct VoxelCBuffer
	{
		Vector3 grid_center;
		Float   data_size;        // voxel half-extent in world space units
		Float   data_size_rcp;    // 1.0 / voxel-half extent
		Uint32    data_res;         // voxel grid resolution
		Float   data_res_rcp;     // 1.0 / voxel grid resolution
		Uint32    num_cones;
		Float   num_cones_rcp;
		Float   max_distance;
		Float   ray_step_size;
		Uint32    mips;
	};

	DECLSPEC_ALIGN(16) struct TerrainCBuffer
	{
		Vector2 texture_scale;
		int ocean_active;
	};

	//Structured Buffers
	struct VoxelType
	{
		Uint32 color_mask;
		Uint32 normal_mask;
	};
	struct LightSBuffer
	{
		Vector4 position;
		Vector4 direction;
		Vector4 color;
		Sint32 active;
		Float range;
		Sint32 type;
		Float outer_cosine;
		Float inner_cosine;
		Sint32 casts_shadows;
		Sint32 use_cascades;
		Sint32 padd;
	};
	struct ClusterAABB
	{
		Vector4 min_point;
		Vector4 max_point;
	};
	struct LightGrid
	{
		Uint32 offset;
		Uint32 light_count;
	};
}