#pragma once
#include "Enums.h"

namespace adria
{

	struct RendererSettings
	{
		F32 ambient_color[3] = { 15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f };
		F32 wind_direction[2] = { 10.0f, 10.0f };

		bool fog = false;
		EFogType fog_type = EFogType::Exponential;
		F32 fog_falloff = 0.005f;
		F32 fog_density = 0.002f;
		F32 fog_start = 100.0f;
		F32 fog_color[3] = { 0.5f,0.6f,0.7f };
		
		bool ibl = false;
		F32 shadow_softness = 1.0f;
		bool shadow_transparent = false;
		F32 split_lambda = 0.25f;
		
		EAntiAliasing anti_aliasing = EAntiAliasing_None;
		
		F32 tone_map_exposure = 1.0f;
		EToneMap tone_map_op = EToneMap::Reinhard;
		//motion blur
		bool motion_blur = false;
		F32 velocity_buffer_scale = 64.0f;
		//ao
		EAmbientOcclusion ambient_occlusion = EAmbientOcclusion::None;
		F32   ssao_power = 4.0f;
		F32   ssao_radius = 1.0f;
		F32   hbao_power = 1.5f;
		F32   hbao_radius = 2.0f;
		//ssr
		bool ssr = false;
		F32 ssr_ray_step = 1.60f;
		F32 ssr_ray_hit_threshold = 2.00f;
		//dof & bokeh
		bool dof = false;
		bool bokeh = false;
		F32 dof_near_blur = 000.0f;
		F32 dof_near = 200.0f;
		F32 dof_far = 400.0f;
		F32 dof_far_blur = 600.0f;
		F32 bokeh_blur_threshold = 0.9f;
		F32 bokeh_lum_threshold = 1.0f;
		F32 bokeh_radius_scale = 25.0f;
		F32 bokeh_color_scale = 1.0f;
		F32 bokeh_fallout = 0.9f;
		//bloom
		bool bloom = false;
		F32 bloom_threshold = 0.25f;
		F32 bloom_scale = 2.0f;
		EBokehType bokeh_type = EBokehType::Hex;
		//clouds
		bool clouds = false;
		F32 crispiness = 43.0f;
		F32 curliness = 3.6f;
		F32 coverage = 0.505f;
		F32 wind_speed = 15.0f;
		F32 light_absorption = 0.003f;
		F32 clouds_bottom_height = 3000.0f;
		F32 clouds_top_height = 10000.0f;
		F32 density_factor = 0.015f;
		F32 cloud_type = 1.0f;
		//ocean
		bool ocean_wireframe = false;
		bool ocean_tesselation = false;
		F32 ocean_color[3] = { 0.0123f, 0.3613f, 0.6867f };
		F32 ocean_choppiness = 1.2f;
		//tiled deferred
		bool use_tiled_deferred = false;
		bool visualize_tiled = false;
		I32 visualize_max_lights = 16;
		//clustered deferred
		bool use_clustered_deferred = false;
		//voxel gi
		bool voxel_gi = false;
		bool voxel_debug = false;
		bool voxel_second_bounce = false;
		F32 voxel_size = 4;
		F32 voxel_center_x = 0;
		F32 voxel_center_y = 0;
		F32 voxel_center_z = 0;
		I32 voxel_num_cones = 2;
		F32 voxel_ray_step_distance = 0.75f;
		F32 voxel_max_distance = 20.0f;
		U32 voxel_mips = 7;
		bool recreate_initial_spectrum = true;
		bool ocean_color_changed = false;
		//sky
		ESkyType sky_type = ESkyType::Skybox;
		F32 sky_color[3] = { 0.53f, 0.81f, 0.92f };
		F32 turbidity = 2.0f;
		F32 ground_albedo = 0.1f;

		//terrain
	};

}