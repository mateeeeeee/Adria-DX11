#pragma once
#include "Enums.h"

namespace adria
{

	struct RendererSettings
	{
		float ambient_color[3] = { 15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f };
		float wind_direction[2] = { 10.0f, 10.0f };

		bool fog = false;
		EFogType fog_type = EFogType::Exponential;
		float fog_falloff = 0.005f;
		float fog_density = 0.002f;
		float fog_start = 100.0f;
		float fog_color[3] = { 0.5f,0.6f,0.7f };
		
		bool ibl = false;
		float shadow_softness = 1.0f;
		bool shadow_transparent = false;
		float split_lambda = 0.25f;
		
		EAntiAliasing anti_aliasing = EAntiAliasing_None;
		
		float tone_map_exposure = 1.0f;
		EToneMap tone_map_op = EToneMap::Reinhard;
		//motion blur
		bool motion_blur = false;
		float velocity_buffer_scale = 64.0f;
		//ao
		EAmbientOcclusion ambient_occlusion = EAmbientOcclusion::None;
		float   ssao_power = 4.0f;
		float   ssao_radius = 1.0f;
		float   hbao_power = 1.5f;
		float   hbao_radius = 2.0f;
		//ssr
		bool ssr = true;
		float ssr_ray_step = 1.60f;
		float ssr_ray_hit_threshold = 2.00f;
		//dof & bokeh
		bool dof = false;
		bool bokeh = false;
		float dof_near_blur = 000.0f;
		float dof_near = 200.0f;
		float dof_far = 400.0f;
		float dof_far_blur = 600.0f;
		float bokeh_blur_threshold = 0.9f;
		float bokeh_lum_threshold = 1.0f;
		float bokeh_radius_scale = 25.0f;
		float bokeh_color_scale = 1.0f;
		float bokeh_fallout = 0.9f;
		//bloom
		bool bloom = false;
		float bloom_threshold = 0.25f;
		float bloom_scale = 2.0f;
		EBokehType bokeh_type = EBokehType::Hex;
		//clouds
		bool clouds = false;
		float crispiness = 43.0f;
		float curliness = 3.6f;
		float coverage = 0.505f;
		float wind_speed = 15.0f;
		float light_absorption = 0.003f;
		float clouds_bottom_height = 3000.0f;
		float clouds_top_height = 10000.0f;
		float density_factor = 0.015f;
		float cloud_type = 1.0f;
		//ocean
		bool ocean_wireframe = false;
		bool ocean_tesselation = false;
		float ocean_color[3] = { 0.0123f, 0.3613f, 0.6867f };
		float ocean_choppiness = 1.2f;
		//tiled deferred
		bool use_tiled_deferred = false;
		bool visualize_tiled = false;
		int32 visualize_max_lights = 16;
		//clustered deferred
		bool use_clustered_deferred = false;
		//voxel gi
		bool voxel_gi = false;
		bool voxel_debug = false;
		bool voxel_second_bounce = false;
		float voxel_size = 4;
		float voxel_center_x = 0;
		float voxel_center_y = 0;
		float voxel_center_z = 0;
		int32 voxel_num_cones = 2;
		float voxel_ray_step_distance = 0.75f;
		float voxel_max_distance = 20.0f;
		uint32 voxel_mips = 7;
		bool recreate_initial_spectrum = true;
		bool ocean_color_changed = false;
		//sky
		ESkyType sky_type = ESkyType::Skybox;
		float sky_color[3] = { 0.53f, 0.81f, 0.92f };
		float turbidity = 2.0f;
		float ground_albedo = 0.1f;
	};

	

}