#pragma once
#include "Enums.h"

namespace adria
{

	struct RendererSettings
	{
		f32 ambient_color[3] = { 15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f };
		f32 wind_direction[2] = { 10.0f, 10.0f };

		bool fog = false;
		FogType fog_type = FogType::eExponential;
		f32 fog_falloff = 0.005f;
		f32 fog_density = 0.002f;
		f32 fog_start = 100.0f;
		f32 fog_color[3] = { 0.5f,0.6f,0.7f };
		
		bool ibl = false;
		f32 shadow_softness = 1.0f;
		bool shadow_transparent = false;
		f32 split_lambda = 0.5f;
		
		AntiAliasing anti_aliasing = AntiAliasing_None;
		
		f32 tone_map_exposure = 1.0f;
		ToneMap tone_map_op = ToneMap::eHable;
		//motion blur
		bool motion_blur = false;
		f32 velocity_buffer_scale = 64.0f;
		//ao
		AmbientOcclusion ambient_occlusion = AmbientOcclusion::eNone;
		f32   ssao_power = 4.0f;
		f32   ssao_radius = 1.0f;
		f32   hbao_power = 1.5f;
		f32   hbao_radius = 2.0f;
		
		//ssr
		bool ssr = false;
		f32 ssr_ray_step = 1.60f;
		f32 ssr_ray_hit_threshold = 2.00f;
		//dof & bokeh
		bool dof = false;
		bool bokeh = false;
		f32 dof_near_blur = 000.0f;
		f32 dof_near = 200.0f;
		f32 dof_far = 400.0f;
		f32 dof_far_blur = 600.0f;
		f32 bokeh_blur_threshold = 0.9f;
		f32 bokeh_lum_threshold = 1.0f;
		f32 bokeh_radius_scale = 25.0f;
		f32 bokeh_color_scale = 1.0f;
		f32 bokeh_fallout = 0.9f;
		//bloom
		bool bloom = false;
		f32 bloom_threshold = 0.25f;
		f32 bloom_scale = 2.0f;
		BokehType bokeh_type = BokehType::eHex;
		//clouds
		bool clouds = false;
		f32 crispiness = 43.0f;
		f32 curliness = 3.6f;
		f32 coverage = 0.505f;
		f32 wind_speed = 15.0f;
		f32 light_absorption = 0.003f;
		f32 clouds_bottom_height = 3000.0f;
		f32 clouds_top_height = 10000.0f;
		f32 density_factor = 0.015f;
		f32 cloud_type = 1.0f;
		//ocean
		bool ocean_wireframe = false;
		bool ocean_tesselation = false;
		f32 ocean_color[3] = { 0.0123f, 0.3613f, 0.6867f };
		f32 ocean_choppiness = 1.2f;
		//tiled deferred
		bool use_tiled_deferred = false;
		bool visualize_tiled = false;
		i32 visualize_max_lights = 16;
		//clustered deferred
		bool use_clustered_deferred = false;
		//voxel gi
		bool voxel_gi = false;
		bool voxel_debug = false;
		bool voxel_second_bounce = false;
		f32 voxel_size = 4;
		f32 voxel_center_x = 0;
		f32 voxel_center_y = 0;
		f32 voxel_center_z = 0;
		i32 voxel_num_cones = 2;
		f32 voxel_ray_step_distance = 0.75f;
		f32 voxel_max_distance = 20.0f;
		u32 voxel_mips = 7;
		bool recreate_initial_spectrum = true;
		bool ocean_color_changed = false;
	};

}