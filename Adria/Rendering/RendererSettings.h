#pragma once
#include "Enums.h"

namespace adria
{

	struct RendererSettings
	{
		Float ambient_color[3] = { 2.0f / 255.0f, 2.0f / 255.0f, 2.0f / 255.0f };
		Float wind_direction[2] = { 10.0f, 10.0f };

		Bool fog = false;
		FogType fog_type = FogType::Exponential;
		Float fog_falloff = 0.005f;
		Float fog_density = 0.002f;
		Float fog_start = 100.0f;
		Float fog_color[3] = { 0.5f,0.6f,0.7f };
		
		Bool ibl = false;
		Float shadow_softness = 1.0f;
		Bool shadow_transparent = false;
		Float split_lambda = 0.25f;
		
		AntiAliasing anti_aliasing = AntiAliasing_None;
		
		Float tone_map_exposure = 1.0f;
		ToneMap tone_map_op = ToneMap::Reinhard;
		//motion blur
		Bool motion_blur = false;
		Float velocity_buffer_scale = 64.0f;
		//ao
		AmbientOcclusion ambient_occlusion = AmbientOcclusion::SSAO;
		Float   ssao_power = 4.0f;
		Float   ssao_radius = 1.0f;
		Float   hbao_power = 1.5f;
		Float   hbao_radius = 2.0f;
		//ssr
		Bool ssr = true;
		Float ssr_ray_step = 1.60f;
		Float ssr_ray_hit_threshold = 2.00f;
		//dof & bokeh
		Bool dof = false;
		Bool bokeh = false;
		Float dof_near_blur = 000.0f;
		Float dof_near = 200.0f;
		Float dof_far = 400.0f;
		Float dof_far_blur = 600.0f;
		Float bokeh_blur_threshold = 0.9f;
		Float bokeh_lum_threshold = 1.0f;
		Float bokeh_radius_scale = 25.0f;
		Float bokeh_color_scale = 1.0f;
		Float bokeh_fallout = 0.9f;
		//bloom
		Bool bloom = false;
		Float bloom_threshold = 0.25f;
		Float bloom_scale = 2.0f;
		BokehType bokeh_type = BokehType::Hex;
		//clouds
		Bool clouds = false;
		Float crispiness = 43.0f;
		Float curliness = 3.6f;
		Float coverage = 0.505f;
		Float wind_speed = 15.0f;
		Float light_absorption = 0.003f;
		Float clouds_bottom_height = 3000.0f;
		Float clouds_top_height = 10000.0f;
		Float density_factor = 0.015f;
		Float cloud_type = 1.0f;
		//ocean
		Bool ocean_wireframe = false;
		Bool ocean_tesselation = false;
		Float ocean_color[3] = { 0.0123f, 0.3613f, 0.6867f };
		Float ocean_choppiness = 1.2f;
		//tiled deferred
		Bool use_tiled_deferred = false;
		Bool visualize_tiled = false;
		Sint32 visualize_max_lights = 16;
		//clustered deferred
		Bool use_clustered_deferred = false;
		//voxel gi
		Bool voxel_gi = false;
		Bool voxel_debug = false;
		Bool voxel_second_bounce = false;
		Float voxel_size = 4;
		Float voxel_center_x = 0;
		Float voxel_center_y = 0;
		Float voxel_center_z = 0;
		Sint32 voxel_num_cones = 2;
		Float voxel_ray_step_distance = 0.75f;
		Float voxel_max_distance = 20.0f;
		Uint32 voxel_mips = 7;
		Bool recreate_initial_spectrum = true;
		Bool ocean_color_changed = false;
		//sky
		SkyType sky_type = SkyType::Skybox;
		Float sky_color[3] = { 0.53f, 0.81f, 0.92f };
		Float turbidity = 2.0f;
		Float ground_albedo = 0.1f;

		//film effects
		Bool lens_distortion_enabled = false;
		Float lens_distortion_intensity = 0.2f;
		Bool chromatic_aberration_enabled = false;
		Float chromatic_aberration_intensity = 10.0f;
		Bool  vignette_enabled = false;
		Float vignette_intensity = 0.5f;
		Bool  film_grain_enabled = false;
		Float film_grain_scale = 3.0f;
		Float film_grain_amount = 0.5f;
		Float film_grain_seed_update_rate = 0.02f;
	};

	

}