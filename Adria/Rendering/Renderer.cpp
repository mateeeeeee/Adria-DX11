#include <map>
#include "Renderer.h"
#include "Camera.h"
#include "Components.h"
#include "ShaderManager.h"
#include "SkyModel.h"
#include "Core/Logger.h"
#include "Core/Paths.h"
#include "Graphics/GfxDevice.h"
#include "Graphics/GfxCommandContext.h"
#include "Graphics/GfxStates.h"
#include "Graphics/GfxScopedAnnotation.h"
#include "Math/Constants.h"
#include "Math/Halton.h"
#include "Utilities/Random.h"
#include "Utilities/StringUtil.h"
#include "DDSTextureLoader.h"

using namespace DirectX;

namespace adria
{
	using namespace tecs;

	namespace
	{
		constexpr Uint32 SHADOW_MAP_SIZE = 2048;
		constexpr Uint32 SHADOW_CUBE_SIZE = 512;
		constexpr Uint32 SHADOW_CASCADE_SIZE = 2048;
		constexpr Uint32 CASCADE_COUNT = 4;

		std::pair<Matrix, Matrix> LightViewProjection_Directional(Light const& light, Camera const& camera, BoundingBox& cull_box)
		{
			BoundingFrustum frustum = camera.Frustum();
			std::array<Vector3, BoundingFrustum::CORNER_COUNT> corners = {};
			frustum.GetCorners(corners.data());

			BoundingSphere frustum_sphere;
			BoundingSphere::CreateFromFrustum(frustum_sphere, frustum);

			Vector3 frustum_center(0, 0, 0);
			for (Uint32 i = 0; i < corners.size(); ++i)
			{
				frustum_center = frustum_center + corners[i];
			}
			frustum_center /= static_cast<Float>(corners.size());

			Float radius = 0.0f;
			for (Vector3 const& corner : corners)
			{
				Float distance = Vector3::Distance(corner, frustum_center);
				radius = std::max(radius, distance);
			}
			radius = std::ceil(radius * 8.0f) / 8.0f;

			Vector3 const max_extents(radius, radius, radius);
			Vector3 const min_extents = -max_extents;
			Vector3 const cascade_extents = max_extents - min_extents;

			Vector4 light_dir = light.direction; light_dir.Normalize();
			Matrix V = XMMatrixLookAtLH(frustum_center, frustum_center + 1.0f * light_dir * radius, Vector3::Up);

			Float l = min_extents.x;
			Float b = min_extents.y;
			Float n = min_extents.z;
			Float r = max_extents.x;
			Float t = max_extents.y;
			Float f = max_extents.z * 1.5f;

			Matrix P = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
			Matrix VP = V * P;
			Vector3 shadow_origin(0, 0, 0);
			shadow_origin = Vector3::Transform(shadow_origin, VP);
			shadow_origin *= (SHADOW_MAP_SIZE / 2.0f);

			Vector3 rounded_origin = XMVectorRound(shadow_origin);
			Vector3 rounded_offset = rounded_origin - shadow_origin;
			rounded_offset *= (2.0f / SHADOW_MAP_SIZE);
			rounded_offset.z = 0.0f;
			P.m[3][0] += rounded_offset.x;
			P.m[3][1] += rounded_offset.y;

			BoundingBox::CreateFromPoints(cull_box, Vector4(l, b, n, 1.0f), Vector4(r, t, f, 1.0f));
			cull_box.Transform(cull_box, V.Invert());

			return { V,P };
		}
		std::pair<Matrix, Matrix> LightViewProjection_Spot(Light const& light, BoundingFrustum& cull_frustum)
		{
			ADRIA_ASSERT(light.type == LightType::Spot);

			Vector3 light_dir = XMVector3Normalize(light.direction);
			Vector3 light_pos = Vector3(light.position);
			Vector3 target_pos = light_pos + light_dir * light.range;

			Matrix V = XMMatrixLookAtLH(light_pos, target_pos, Vector3::Up);
			static const Float shadow_near = 0.5f;
			Float fov_angle = 2.0f * acos(light.outer_cosine);
			Matrix P = XMMatrixPerspectiveFovLH(fov_angle, 1.0f, shadow_near, light.range);

			cull_frustum = BoundingFrustum(P);
			cull_frustum.Transform(cull_frustum, V.Invert());

			return { V,P };
		}
		std::pair<Matrix, Matrix> LightViewProjection_Point(Light const& light, Uint32 face_index, BoundingFrustum& cull_frustum)
		{
			static Float const shadow_near = 0.5f;
			Matrix P = XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), 1.0f, shadow_near, light.range);

			Vector3 light_pos = Vector3(light.position);
			Matrix V{};
			Vector3 target{};
			Vector3 up{};
			switch (face_index)
			{
			case 0:
				target = light_pos + Vector3(1, 0, 0);
				up = Vector3(0.0f, 1.0f, 0.0f);
				V = XMMatrixLookAtLH(light_pos, target, up);
				break;
			case 1:
				target = light_pos + Vector3(-1, 0, 0);
				up = Vector3(0.0f, 1.0f, 0.0f);
				V = XMMatrixLookAtLH(light_pos, target, up);
				break;
			case 2:
				target = light_pos + Vector3(0, 1, 0);
				up = Vector3(0.0f, 0.0f, -1.0f);
				V = XMMatrixLookAtLH(light_pos, target, up);
				break;
			case 3:
				target = light_pos + Vector3(0, -1, 0);
				up = Vector3(0.0f, 0.0f, 1.0f);
				V = XMMatrixLookAtLH(light_pos, target, up);
				break;
			case 4:
				target = light_pos + Vector3(0, 0, 1);
				up = Vector3(0.0f, 1.0f, 0.0f);
				V = XMMatrixLookAtLH(light_pos, target, up);
				break;
			case 5:
				target = light_pos + Vector3(0, 0, -1);
				up = Vector3(0.0f, 1.0f, 0.0f);
				V = XMMatrixLookAtLH(light_pos, target, up);
				break;
			default:
				ADRIA_ASSERT(false && "Invalid face index!");
			}
			cull_frustum = BoundingFrustum(P);
			cull_frustum.Transform(cull_frustum, V.Invert());
			return { V,P };
		}
		std::pair<Matrix, Matrix> LightViewProjection_Cascades(Light const& light, Camera const& camera, Matrix const& projection_matrix, BoundingBox& cull_box)
		{
			static Float const far_factor = 1.5f;
			static Float const light_distance_factor = 1.0f;

			BoundingFrustum frustum(projection_matrix);
			frustum.Transform(frustum, camera.View().Invert());
			std::array<Vector3, BoundingFrustum::CORNER_COUNT> corners{};
			frustum.GetCorners(corners.data());

			Vector3 frustum_center(0, 0, 0);
			for (Vector3 const& corner : corners)
			{
				frustum_center = frustum_center + corner;
			}
			frustum_center /= static_cast<Float>(corners.size());

			Float radius = 0.0f;
			for (Vector3 const& corner : corners)
			{
				Float distance = Vector3::Distance(corner, frustum_center);
				radius = std::max(radius, distance);
			}
			radius = std::ceil(radius * 8.0f) / 8.0f;

			Vector3 const max_extents(radius, radius, radius);
			Vector3 const min_extents = -max_extents;
			Vector3 const cascade_extents = max_extents - min_extents;

			Vector3 light_dir = XMVector3Normalize(light.direction);
			Matrix V = XMMatrixLookAtLH(frustum_center, frustum_center + light_distance_factor * light_dir * radius, Vector3::Up);

			Float l = min_extents.x;
			Float b = min_extents.y;
			Float n = min_extents.z - far_factor * radius;
			Float r = max_extents.x;
			Float t = max_extents.y;
			Float f = max_extents.z * far_factor;

			Matrix P = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
			Matrix VP = V * P;
			Vector3 shadow_origin(0, 0, 0);
			shadow_origin = Vector3::Transform(shadow_origin, VP);
			shadow_origin *= (SHADOW_CASCADE_SIZE / 2.0f);

			Vector3 rounded_origin = XMVectorRound(shadow_origin);
			Vector3 rounded_offset = rounded_origin - shadow_origin;
			rounded_offset *= (2.0f / SHADOW_CASCADE_SIZE);
			rounded_offset.z = 0.0f;
			P.m[3][0] += rounded_offset.x;
			P.m[3][1] += rounded_offset.y;

			BoundingBox::CreateFromPoints(cull_box, Vector4(l, b, n, 1.0f), Vector4(r, t, f, 1.0f));
			cull_box.Transform(cull_box, V.Invert());
			return { V,P };
		}
		std::pair<Matrix, Matrix> LightViewProjection_Directional(Light const& light, BoundingSphere const& scene_bounding_sphere, BoundingBox& cull_box)
		{
			static const Vector3 sphere_center = scene_bounding_sphere.Center;

			Vector3 light_dir = XMVector3Normalize(light.direction);
			Vector3 light_pos = -1.0f * scene_bounding_sphere.Radius * light_dir + sphere_center;
			Vector3 target_pos = XMLoadFloat3(&scene_bounding_sphere.Center);
			Matrix V = XMMatrixLookAtLH(light_pos, target_pos, Vector3::Up);
			Vector3 sphere_centerLS = Vector3::Transform(target_pos, V);

			Float l = sphere_centerLS.x - scene_bounding_sphere.Radius;
			Float b = sphere_centerLS.y - scene_bounding_sphere.Radius;
			Float n = sphere_centerLS.z - scene_bounding_sphere.Radius;
			Float r = sphere_centerLS.x + scene_bounding_sphere.Radius;
			Float t = sphere_centerLS.y + scene_bounding_sphere.Radius;
			Float f = sphere_centerLS.z + scene_bounding_sphere.Radius;

			Matrix P = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

			BoundingBox::CreateFromPoints(cull_box, Vector4(l, b, n, 1.0f), Vector4(r, t, f, 1.0f));
			cull_box.Transform(cull_box, V.Invert());

			return { V,P };
		}

		std::array<Matrix, CASCADE_COUNT> RecalculateProjectionMatrices(Camera const& camera, Float split_lambda, std::array<Float, CASCADE_COUNT>& split_distances)
		{
			Float camera_near = camera.Near();
			Float camera_far = camera.Far();
			Float fov = camera.Fov();
			Float ar = camera.AspectRatio();
			Float f = 1.0f / CASCADE_COUNT;
			for (Uint32 i = 0; i < split_distances.size(); i++)
			{
				Float fi = (i + 1) * f;
				Float l = camera_near * pow(camera_far / camera_near, fi);
				Float u = camera_near + (camera_far - camera_near) * fi;
				split_distances[i] = l * split_lambda + u * (1.0f - split_lambda);
			}

			std::array<Matrix, CASCADE_COUNT> projectionMatrices{};
			projectionMatrices[0] = XMMatrixPerspectiveFovLH(fov, ar, camera_near, split_distances[0]);
			for (Uint32 i = 1; i < projectionMatrices.size(); ++i)
				projectionMatrices[i] = XMMatrixPerspectiveFovLH(fov, ar, split_distances[i - 1], split_distances[i]);
			return projectionMatrices;
		}

		Float GaussianDistribution(Float x, Float sigma)
		{
			static const Float square_root_of_two_pi = sqrt(pi_times_2<Float>);
			return expf((-x * x) / (2 * sigma * sigma)) / (sigma * square_root_of_two_pi);
		}
		template<Sint16 N>
		std::array<Float, 2 * N + 1> GaussKernel(Float sigma)
		{
			std::array<Float, 2 * N + 1> gauss{};
			Float sum = 0.0f;
			for (Sint16 i = -N; i <= N; ++i)
			{
				gauss[i + N] = GaussianDistribution(i * 1.0f, sigma);
				sum += gauss[i + N];
			}
			for (Sint16 i = -N; i <= N; ++i)
			{
				gauss[i + N] /= sum;
			}

			return gauss;
		}

	}

	Renderer::Renderer(registry& reg, GfxDevice* gfx, Uint32 width, Uint32 height)
		: width(width), height(height), reg(reg), gfx(gfx), particle_renderer(gfx), picker(gfx)
	{
		g_GfxProfiler.Initialize(gfx);
		CreateRenderStates();
		CreateBuffers();
		CreateSamplers();
		LoadTextures();

		CreateOtherResources();
		CreateResolutionDependentResources(width, height);
	}
	Renderer::~Renderer()
	{
		for (auto& clouds_texture : clouds_textures) clouds_texture->Release();
		g_GfxProfiler.Destroy();
	}

	void Renderer::Update(Float dt)
	{
		current_dt = dt;
		UpdateLights();
		UpdateTerrainData();
		UpdateVoxelData();
		CameraFrustumCulling();
		UpdateCBuffers(dt);
		UpdateWeather(dt);
		UpdateOcean(dt);
		UpdateParticles(dt);
	}
	void Renderer::SetSceneViewportData(SceneViewport const& vp)
	{
		current_scene_viewport = vp;
	}
	void Renderer::Render(RendererSettings const& _settings)
	{
		renderer_settings = _settings;
		if (renderer_settings.ibl && !ibl_textures_generated) CreateIBLTextures();

		PassGBuffer();
		PassDecals();

		if (pick_in_current_frame) PassPicking();

		if(!renderer_settings.voxel_debug)
		{
			if (renderer_settings.ambient_occlusion == AmbientOcclusion::SSAO) PassSSAO();
			else if (renderer_settings.ambient_occlusion == AmbientOcclusion::HBAO) PassHBAO();
		
			PassAmbient();
			PassDeferredLighting();
		
			if (renderer_settings.use_tiled_deferred) PassDeferredTiledLighting();
			else if (renderer_settings.use_clustered_deferred) PassDeferredClusteredLighting();
		
			PassForward();
			PassParticles();
		}

		if (renderer_settings.voxel_gi)
		{
			PassVoxelize();
			if (renderer_settings.voxel_debug) PassVoxelizeDebug();
			else PassVoxelGI();
		}

		PassPostprocessing();
	}
	void Renderer::ResolveToBackbuffer()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		if (renderer_settings.anti_aliasing & AntiAliasing_FXAA)
		{
			command_context->BeginRenderPass(fxaa_pass);
			PassToneMap();
			command_context->EndRenderPass();

			gfx->SetBackbuffer(); 
			PassFXAA();
		}
		else
		{
			gfx->SetBackbuffer();
			PassToneMap();
		}

	}
	void Renderer::ResolveToOffscreenTexture()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		if (renderer_settings.anti_aliasing & AntiAliasing_FXAA)
		{
			command_context->BeginRenderPass(fxaa_pass);
			PassToneMap();
			command_context->EndRenderPass();
			
			command_context->BeginRenderPass(offscreen_resolve_pass);
			PassFXAA();
			command_context->EndRenderPass();
		}
		else
		{
			command_context->BeginRenderPass(offscreen_resolve_pass);
			PassToneMap();
			command_context->EndRenderPass();
		}
	}
	void Renderer::OnResize(Uint32 w, Uint32 h)
	{
		width = w, height = h;
		if (width != 0 || height != 0)
		{
			CreateResolutionDependentResources(width, height);
		}
	}
	void Renderer::OnLeftMouseClicked()
	{
		if (current_scene_viewport.scene_viewport_focused)
		{
			pick_in_current_frame = true;
		}
	}

	GfxTexture const* Renderer::GetOffscreenTexture() const
	{
		return offscreen_ldr_render_target.get();
	}
	void Renderer::Tick(Camera const* _camera)
	{
		BindGlobals();
		g_GfxProfiler.NewFrame();

		camera = _camera;
		frame_cbuf_data.global_ambient = Vector4{ renderer_settings.ambient_color[0], renderer_settings.ambient_color[1], renderer_settings.ambient_color[2], 1.0f };

		static Uint32 frame_index = 0;
		Float jitter_x = 0.0f, jitter_y = 0.0f;
		if (HasAllFlags(renderer_settings.anti_aliasing, AntiAliasing_TAA))
		{
			constexpr HaltonSequence<16, 2> x;
			constexpr HaltonSequence<16, 3> y;
			jitter_x = x[frame_index % 16];
			jitter_y = y[frame_index % 16];
			jitter_x = ((jitter_x - 0.5f) / width) * 2;
			jitter_y = ((jitter_y - 0.5f) / height) * 2;
		}

		frame_cbuf_data.camera_near = camera->Near();
		frame_cbuf_data.camera_far = camera->Far();
		frame_cbuf_data.camera_jitter_x = jitter_x;
		frame_cbuf_data.camera_jitter_y = jitter_y;
		frame_cbuf_data.camera_position = Vector4(camera->Position());
		frame_cbuf_data.camera_forward = Vector4(camera->Forward());
		frame_cbuf_data.view = camera->View();
		frame_cbuf_data.projection = camera->Proj();
		frame_cbuf_data.viewprojection = camera->ViewProj();
		frame_cbuf_data.inverse_view = camera->View().Invert();
		frame_cbuf_data.inverse_projection = camera->Proj().Invert();
		frame_cbuf_data.inverse_view_projection = camera->ViewProj().Invert();
		frame_cbuf_data.screen_resolution_x = (Float)width;
		frame_cbuf_data.screen_resolution_y = (Float)height;
		frame_cbuf_data.mouse_normalized_coords_x = (current_scene_viewport.mouse_position_x - current_scene_viewport.scene_viewport_pos_x) / current_scene_viewport.scene_viewport_size_x;
		frame_cbuf_data.mouse_normalized_coords_y = (current_scene_viewport.mouse_position_y - current_scene_viewport.scene_viewport_pos_y) / current_scene_viewport.scene_viewport_size_y;

		frame_cbuffer->Update(gfx->GetCommandContext(), frame_cbuf_data);

		if (camera->Proj() != frame_cbuf_data.previous_projection)
		{
			recreate_clusters = true;
		}

		//set for next frame
		frame_cbuf_data.previous_view = camera->View();
		frame_cbuf_data.previous_projection = camera->Proj();
		frame_cbuf_data.previous_view_projection = camera->ViewProj(); 
		++frame_index;
	}
	PickingData Renderer::GetLastPickingData() const
	{
		return last_picking_data;
	}
	std::vector<Timestamp> Renderer::GetProfilerResults()
	{
		return g_GfxProfiler.GetProfilingResults();
	}

	void Renderer::LoadTextures()
	{
		TextureHandle tex_handle{};
		tex_handle = g_TextureManager.LoadTexture(paths::TexturesDir + "lensflare/flare0.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		tex_handle = g_TextureManager.LoadTexture(paths::TexturesDir + "lensflare/flare1.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		tex_handle = g_TextureManager.LoadTexture(paths::TexturesDir + "lensflare/flare2.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		tex_handle = g_TextureManager.LoadTexture(paths::TexturesDir + "lensflare/flare3.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		tex_handle = g_TextureManager.LoadTexture(paths::TexturesDir + "lensflare/flare4.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		tex_handle = g_TextureManager.LoadTexture(paths::TexturesDir + "lensflare/flare5.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		tex_handle = g_TextureManager.LoadTexture(paths::TexturesDir + "lensflare/flare6.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		clouds_textures.resize(3);

		ID3D11Device* device = gfx->GetDevice();
		ID3D11DeviceContext* context = gfx->GetContext();

		std::wstring cloud_textures_path = ToWideString(paths::TexturesDir + "clouds/");
		std::wstring weather = cloud_textures_path + L"weather.dds";
		std::wstring cloud = cloud_textures_path   + L"cloud.dds";
		std::wstring worley = cloud_textures_path + L"worley.dds";
		HRESULT hr = CreateDDSTextureFromFile(device, context, weather.c_str(), nullptr, &clouds_textures[0]);
		GFX_CHECK_HR(hr);
		hr = CreateDDSTextureFromFile(device, context, cloud.c_str(), nullptr, &clouds_textures[1]);
		GFX_CHECK_HR(hr);
		hr = CreateDDSTextureFromFile(device, context, worley.c_str(), nullptr, &clouds_textures[2]);
		GFX_CHECK_HR(hr);

		foam_handle		= g_TextureManager.LoadTexture(paths::TexturesDir + "foam.jpg");
		perlin_handle	= g_TextureManager.LoadTexture(paths::TexturesDir + "perlin.dds");

		hex_bokeh_handle = g_TextureManager.LoadTexture(paths::TexturesDir + "bokeh/Bokeh_Hex.dds");
		oct_bokeh_handle = g_TextureManager.LoadTexture(paths::TexturesDir + "bokeh/Bokeh_Oct.dds");
		circle_bokeh_handle = g_TextureManager.LoadTexture(paths::TexturesDir + "bokeh/Bokeh_Circle.dds");
		cross_bokeh_handle = g_TextureManager.LoadTexture(paths::TexturesDir + "bokeh/Bokeh_Cross.dds");

		lut_tony_mcmapface_handle = g_TextureManager.LoadTexture(paths::TexturesDir + "tony_mc_mapface.dds");
	}
	void Renderer::CreateBuffers()
	{
		frame_cbuffer = std::make_unique<GfxConstantBuffer<FrameCBuffer>>(gfx);
		light_cbuffer = std::make_unique<GfxConstantBuffer<LightCBuffer>>(gfx);
		object_cbuffer = std::make_unique<GfxConstantBuffer<ObjectCBuffer>>(gfx);
		material_cbuffer = std::make_unique<GfxConstantBuffer<MaterialCBuffer>>(gfx);
		shadow_cbuffer = std::make_unique<GfxConstantBuffer<ShadowCBuffer>>(gfx);
		postprocess_cbuffer = std::make_unique<GfxConstantBuffer<PostprocessCBuffer>>(gfx);
		compute_cbuffer = std::make_unique<GfxConstantBuffer<ComputeCBuffer>>(gfx);
		weather_cbuffer = std::make_unique<GfxConstantBuffer<WeatherCBuffer>>(gfx);
		voxel_cbuffer = std::make_unique<GfxConstantBuffer<VoxelCBuffer>>(gfx);
		terrain_cbuffer = std::make_unique<GfxConstantBuffer<TerrainCBuffer>>(gfx);

		GfxBufferDesc bokeh_indirect_draw_buffer_desc{};
		bokeh_indirect_draw_buffer_desc.size = 4 * sizeof(Uint32);
		bokeh_indirect_draw_buffer_desc.misc_flags = GfxBufferMiscFlag::IndirectArgs;
		Uint32 buffer_init[4] = { 0, 1, 0, 0 };
		bokeh_indirect_draw_buffer = std::make_unique<GfxBuffer>(gfx, bokeh_indirect_draw_buffer_desc, buffer_init);

		static constexpr Uint32 CLUSTER_COUNT = CLUSTER_SIZE_X * CLUSTER_SIZE_Y * CLUSTER_SIZE_Z;
		voxels = std::make_unique<GfxBuffer>(gfx, StructuredBufferDesc<VoxelType>(VOXEL_RESOLUTION * VOXEL_RESOLUTION * VOXEL_RESOLUTION));
		clusters = std::make_unique<GfxBuffer>(gfx, StructuredBufferDesc<ClusterAABB>(CLUSTER_COUNT));
		light_counter = std::make_unique<GfxBuffer>(gfx, StructuredBufferDesc<Uint32>(1));
		light_list = std::make_unique<GfxBuffer>(gfx, StructuredBufferDesc<Uint32>(CLUSTER_COUNT * CLUSTER_MAX_LIGHTS));
		light_grid = std::make_unique<GfxBuffer>(gfx, StructuredBufferDesc<LightGrid>(CLUSTER_COUNT));

		voxels->CreateSRV();
		voxels->CreateUAV();
		clusters->CreateSRV();
		clusters->CreateUAV();
		light_counter->CreateSRV();
		light_counter->CreateUAV();
		light_list->CreateSRV();
		light_list->CreateUAV();
		light_grid->CreateSRV();
		light_grid->CreateUAV();
		
		const SimpleVertex cube_vertices[] = 
		{
			Vector3{ -0.5f, -0.5f,  0.5f },
			Vector3{  0.5f, -0.5f,  0.5f },
			Vector3{  0.5f,  0.5f,  0.5f },
			Vector3{ -0.5f,  0.5f,  0.5f },
			Vector3{ -0.5f, -0.5f, -0.5f },
			Vector3{  0.5f, -0.5f, -0.5f },
			Vector3{  0.5f,  0.5f, -0.5f },
			Vector3{ -0.5f,  0.5f, -0.5f }
		};

		const Uint16 cube_indices[] = 
		{
			0, 1, 2,
			2, 3, 0,
			1, 5, 6,
			6, 2, 1,
			7, 6, 5,
			5, 4, 7,
			4, 0, 3,
			3, 7, 4,
			4, 5, 1,
			1, 0, 4,
			3, 2, 6,
			6, 7, 3
		};

		cube_vb = std::make_unique<GfxBuffer>(gfx, VertexBufferDesc(ARRAYSIZE(cube_vertices), sizeof(SimpleVertex)), cube_vertices);
		cube_ib = std::make_unique<GfxBuffer>(gfx, IndexBufferDesc(ARRAYSIZE(cube_indices), true), cube_indices);

		const Uint16 aabb_indices[] = {
			0, 1,
			1, 2,
			2, 3,
			3, 0,

			0, 4,
			1, 5,
			2, 6,
			3, 7,

			4, 5,
			5, 6,
			6, 7,
			7, 4
		};
		aabb_wireframe_ib = std::make_unique<GfxBuffer>(gfx, IndexBufferDesc(ARRAYSIZE(aabb_indices), true), aabb_indices);
	}
	void Renderer::CreateSamplers()
	{
		linear_wrap_sampler = std::make_unique<GfxSampler>(gfx, SamplerDesc(GfxFilter::MIN_MAG_MIP_LINEAR, GfxTextureAddressMode::Wrap));
		point_wrap_sampler = std::make_unique<GfxSampler>(gfx, SamplerDesc(GfxFilter::MIN_MAG_MIP_POINT, GfxTextureAddressMode::Wrap));
		linear_border_sampler = std::make_unique<GfxSampler>(gfx, SamplerDesc(GfxFilter::MIN_MAG_LINEAR_MIP_POINT, GfxTextureAddressMode::Border));
		linear_clamp_sampler = std::make_unique<GfxSampler>(gfx, SamplerDesc(GfxFilter::MIN_MAG_MIP_LINEAR, GfxTextureAddressMode::Clamp));
		point_clamp_sampler = std::make_unique<GfxSampler>(gfx, SamplerDesc(GfxFilter::MIN_MAG_MIP_POINT, GfxTextureAddressMode::Clamp));
		anisotropic_sampler = std::make_unique<GfxSampler>(gfx, SamplerDesc(GfxFilter::ANISOTROPIC, GfxTextureAddressMode::Wrap));
		
		GfxSamplerDesc comparison_sampler_desc{};
		comparison_sampler_desc.filter = GfxFilter::COMPARISON_MIN_MAG_MIP_LINEAR;
		comparison_sampler_desc.addressU = comparison_sampler_desc.addressV = comparison_sampler_desc.addressW = GfxTextureAddressMode::Border;
		comparison_sampler_desc.border_color[0] = comparison_sampler_desc.border_color[1] = comparison_sampler_desc.border_color[2] = comparison_sampler_desc.border_color[3] = 1.0f;
		comparison_sampler_desc.comparison_func = GfxComparisonFunc::LessEqual;
		shadow_sampler = std::make_unique<GfxSampler>(gfx, comparison_sampler_desc);
	}
	void Renderer::CreateRenderStates()
	{
		additive_blend = std::make_unique<GfxBlendState>(gfx, AdditiveBlendStateDesc());
		alpha_blend = std::make_unique<GfxBlendState>(gfx, AlphaBlendStateDesc());

		leq_depth = std::make_unique<GfxDepthStencilState>(gfx, DefaultDepthDesc());
		no_depth_test = std::make_unique<GfxDepthStencilState>(gfx, NoneDepthDesc());

		cull_none = std::make_unique<GfxRasterizerState>(gfx, CullNoneDesc());
		
		GfxRasterizerStateDesc shadow_depth_bias_state{};
		shadow_depth_bias_state.cull_mode = GfxCullMode::Front;
		shadow_depth_bias_state.fill_mode = GfxFillMode::Solid;
		shadow_depth_bias_state.depth_clip_enable = true;
		shadow_depth_bias_state.depth_bias = 8500;
		shadow_depth_bias_state.depth_bias_clamp = 0.0f;
		shadow_depth_bias_state.slope_scaled_depth_bias = 1.0f;
		shadow_depth_bias = std::make_unique<GfxRasterizerState>(gfx, shadow_depth_bias_state);

		wireframe = std::make_unique<GfxRasterizerState>(gfx, WireframeDesc());
	}

	void Renderer::CreateResolutionDependentResources(Uint32 width, Uint32 height)
	{
		CreateBokehViews(width, height);
		CreateRenderTargets(width, height);
		CreateGBuffer(width, height);
		CreateAOTexture(width, height);
		CreateComputeTextures(width, height);
		CreateRenderPasses(width, height);
	}
	void Renderer::CreateOtherResources()
	{
		{
			GfxTextureDesc depth_map_desc{};
			depth_map_desc.width = SHADOW_MAP_SIZE;
			depth_map_desc.height = SHADOW_MAP_SIZE;
			depth_map_desc.format = GfxFormat::R32_TYPELESS;
			depth_map_desc.bind_flags = GfxBindFlag::DepthStencil | GfxBindFlag::ShaderResource;
			shadow_depth_map = std::make_unique<GfxTexture>(gfx, depth_map_desc);

			GfxTextureDesc depth_cubemap_desc{};
			depth_cubemap_desc.width = SHADOW_CUBE_SIZE;
			depth_cubemap_desc.height = SHADOW_CUBE_SIZE;
			depth_cubemap_desc.array_size = 6;
			depth_cubemap_desc.format = GfxFormat::R32_TYPELESS;
			depth_cubemap_desc.bind_flags = GfxBindFlag::DepthStencil | GfxBindFlag::ShaderResource;
			depth_cubemap_desc.misc_flags = GfxTextureMiscFlag::TextureCube;
			shadow_depth_cubemap = std::make_unique<GfxTexture>(gfx, depth_cubemap_desc);
			for (Uint32 i = 0; i < 6; ++i)
			{
				GfxTextureSubresourceDesc dsv_desc{};
				dsv_desc.first_mip = 0;
				dsv_desc.slice_count = 1;
				dsv_desc.first_slice = i;
				size_t j = shadow_depth_cubemap->CreateDSV(&dsv_desc);
				ADRIA_ASSERT(j == i + 1);
			}

			GfxTextureDesc depth_cascade_maps_desc{};
			depth_cascade_maps_desc.width = SHADOW_CASCADE_SIZE;
			depth_cascade_maps_desc.height = SHADOW_CASCADE_SIZE;
			depth_cascade_maps_desc.array_size = CASCADE_COUNT;
			depth_cascade_maps_desc.format = GfxFormat::R32_TYPELESS;
			depth_cascade_maps_desc.bind_flags = GfxBindFlag::DepthStencil | GfxBindFlag::ShaderResource;
			shadow_cascade_maps = std::make_unique<GfxTexture>(gfx, depth_cascade_maps_desc);
			for (size_t i = 0; i < CASCADE_COUNT; ++i)
			{
				GfxTextureSubresourceDesc dsv_desc{};
				dsv_desc.first_mip = 0;
				dsv_desc.slice_count = 1;
				dsv_desc.first_slice = (Uint32)i;
				size_t j = shadow_cascade_maps->CreateDSV(&dsv_desc);
				ADRIA_ASSERT(j == i + 1);
			}
		}

		//ao random
		{
			std::vector<Float> random_texture_data;
			random_texture_data.reserve(AO_NOISE_DIM * AO_NOISE_DIM * 4);
			RealRandomGenerator rand_float{ 0.0f, 1.0f };
			for (Uint32 i = 0; i < ssao_kernel.size(); i++)
			{
				Vector4 offset(2 * rand_float() - 1, 2 * rand_float() - 1, rand_float(), 0.0f);
				offset.Normalize();
				offset *= rand_float();
				Float scale = static_cast<Float>(i) / ssao_kernel.size();
				scale = std::lerp(0.1f, 1.0f, scale * scale);
				offset *= scale;
				ssao_kernel[i] = offset;
			}

			for (Sint32 i = 0; i < AO_NOISE_DIM * AO_NOISE_DIM; i++)
			{
				random_texture_data.push_back(rand_float());
				random_texture_data.push_back(rand_float());
				random_texture_data.push_back(0.0f);
				random_texture_data.push_back(1.0f);
			}

			GfxTextureDesc random_tex_desc{};
			random_tex_desc.width = AO_NOISE_DIM;
			random_tex_desc.height = AO_NOISE_DIM;
			random_tex_desc.format = GfxFormat::R32G32B32A32_FLOAT;
			random_tex_desc.bind_flags = GfxBindFlag::ShaderResource;

			GfxTextureInitialData init_data{};
			init_data.pSysMem = (void*)random_texture_data.data();
			init_data.SysMemPitch = AO_NOISE_DIM * 4 * sizeof(Float);
			ssao_random_texture = std::make_unique<GfxTexture>(gfx, random_tex_desc, &init_data);
			ssao_random_texture->CreateSRV();

			random_texture_data.clear();
			for (Sint32 i = 0; i < AO_NOISE_DIM * AO_NOISE_DIM; i++)
			{
				Float rand = rand_float() * pi<Float> *2.0f;
				random_texture_data.push_back(sin(rand));
				random_texture_data.push_back(cos(rand));
				random_texture_data.push_back(rand_float());
				random_texture_data.push_back(rand_float());
			}
			init_data.pSysMem = (void*)random_texture_data.data();
			hbao_random_texture = std::make_unique<GfxTexture>(gfx, random_tex_desc, &init_data);
			hbao_random_texture->CreateSRV();
		}

		//ocean
		{
			GfxTextureDesc desc{};
			desc.width = RESOLUTION;
			desc.height = RESOLUTION;
			desc.format = GfxFormat::R32_FLOAT;
			desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::UnorderedAccess;
			ocean_initial_spectrum = std::make_unique<GfxTexture>(gfx, desc);

			std::vector<Float> ping_array(RESOLUTION * RESOLUTION);
			RealRandomGenerator rand_float{ 0.0f, 1.0f };
			for (size_t i = 0; i < ping_array.size(); ++i) ping_array[i] = rand_float() * 2.f * pi<Float>;
			ping_pong_phase_textures[!pong_phase] = std::make_unique<GfxTexture>(gfx, desc);

			GfxTextureInitialData init_data{};
			init_data.pSysMem = ping_array.data();
			init_data.SysMemPitch = RESOLUTION * sizeof(Float);
			ping_pong_phase_textures[pong_phase] = std::make_unique<GfxTexture>(gfx, desc, &init_data);

			desc.format = GfxFormat::R32G32B32A32_FLOAT;
			ping_pong_spectrum_textures[pong_spectrum] = std::make_unique<GfxTexture>(gfx, desc);
			ping_pong_spectrum_textures[!pong_spectrum] = std::make_unique<GfxTexture>(gfx, desc);
			ocean_normal_map = std::make_unique<GfxTexture>(gfx, desc);
		}

		//voxel 
		{
			GfxTextureDesc voxel_desc{};
			voxel_desc.type = TextureType_3D;
			voxel_desc.width = VOXEL_RESOLUTION;
			voxel_desc.height = VOXEL_RESOLUTION;
			voxel_desc.depth = VOXEL_RESOLUTION;
			voxel_desc.misc_flags = GfxTextureMiscFlag::GenerateMips;
			voxel_desc.mip_levels = 0;
			voxel_desc.bind_flags = GfxBindFlag::UnorderedAccess | GfxBindFlag::ShaderResource | GfxBindFlag::RenderTarget;
			voxel_desc.format = GfxFormat::R16G16B16A16_FLOAT;

			voxel_texture = std::make_unique<GfxTexture>(gfx, voxel_desc);
			voxel_texture_second_bounce = std::make_unique<GfxTexture>(gfx, voxel_desc);
		}
	}

	void Renderer::CreateBokehViews(Uint32 width, Uint32 height)
	{
		Uint32 const max_bokeh = width * height;

		GfxBufferDesc bokeh_buffer_desc{};
		bokeh_buffer_desc.stride = 8 * sizeof(Float);
		bokeh_buffer_desc.size = bokeh_buffer_desc.stride * max_bokeh;
		bokeh_buffer_desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::UnorderedAccess;
		bokeh_buffer_desc.misc_flags = GfxBufferMiscFlag::BufferStructured;
		bokeh_buffer_desc.resource_usage = GfxResourceUsage::Default;

		bokeh_buffer = std::make_unique<GfxBuffer>(gfx, bokeh_buffer_desc);

		GfxBufferSubresourceDesc uav_desc{};
		uav_desc.uav_flags = UAV_Append;
		bokeh_buffer->CreateUAV(&uav_desc);
		bokeh_buffer->CreateSRV();
	}
	void Renderer::CreateRenderTargets(Uint32 width, Uint32 height)
	{
		GfxTextureDesc render_target_desc{};
		render_target_desc.width = width;
		render_target_desc.height = height;
		render_target_desc.format = GfxFormat::R16G16B16A16_FLOAT;
		render_target_desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::RenderTarget;

		hdr_render_target = std::make_unique<GfxTexture>(gfx, render_target_desc);
		prev_hdr_render_target = std::make_unique<GfxTexture>(gfx, render_target_desc);
		sun_target = std::make_unique<GfxTexture>(gfx, render_target_desc);

		render_target_desc.bind_flags |= GfxBindFlag::UnorderedAccess;
		postprocess_textures[0] = std::make_unique<GfxTexture>(gfx, render_target_desc);
		postprocess_textures[1] = std::make_unique<GfxTexture>(gfx, render_target_desc);

		GfxTextureDesc depth_target_desc{};
		depth_target_desc.width = width;
		depth_target_desc.height = height;
		depth_target_desc.format = GfxFormat::R32_TYPELESS;
		depth_target_desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::DepthStencil;
		depth_target = std::make_unique<GfxTexture>(gfx, depth_target_desc);
		
		GfxTextureDesc fxaa_source_desc{};
		fxaa_source_desc.width = width;
		fxaa_source_desc.height = height;
		fxaa_source_desc.format = GfxFormat::R8G8B8A8_UNORM;
		fxaa_source_desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::RenderTarget;
		fxaa_texture = std::make_unique<GfxTexture>(gfx, fxaa_source_desc);

		GfxTextureDesc offscreeen_desc = fxaa_source_desc;
		offscreen_ldr_render_target = std::make_unique<GfxTexture>(gfx, offscreeen_desc);

		GfxTextureDesc uav_target_desc{};
		uav_target_desc.width = width;
		uav_target_desc.height = height;
		uav_target_desc.format = GfxFormat::R16G16B16A16_FLOAT;
		uav_target_desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::UnorderedAccess;
		uav_target = std::make_unique<GfxTexture>(gfx, uav_target_desc);

		GfxTextureDesc tiled_debug_desc{};
		tiled_debug_desc.width = width;
		tiled_debug_desc.height = height;
		tiled_debug_desc.format = GfxFormat::R16G16B16A16_FLOAT;
		tiled_debug_desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::UnorderedAccess;
		debug_tiled_texture = std::make_unique<GfxTexture>(gfx, tiled_debug_desc);

		GfxTextureDesc velocity_buffer_desc{};
		velocity_buffer_desc.width = width;
		velocity_buffer_desc.height = height;
		velocity_buffer_desc.format = GfxFormat::R16G16_FLOAT;
		velocity_buffer_desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::RenderTarget;
		velocity_buffer = std::make_unique<GfxTexture>(gfx, velocity_buffer_desc);
	}
	void Renderer::CreateGBuffer(Uint32 width, Uint32 height)
	{
		gbuffer.clear();
		
		GfxTextureDesc render_target_desc{};
		render_target_desc.width = width;
		render_target_desc.height = height;
		render_target_desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::RenderTarget;
		
		for (Uint32 i = 0; i < GBufferSlot_Count; ++i)
		{
			render_target_desc.format = GBUFFER_FORMAT[i];
			gbuffer.push_back(std::make_unique<GfxTexture>(gfx, render_target_desc));
		}
	}
	void Renderer::CreateAOTexture(Uint32 width, Uint32 height)
	{
		GfxTextureDesc ao_tex_desc{};
		ao_tex_desc.width = width;
		ao_tex_desc.height = height;
		ao_tex_desc.format = GfxFormat::R8_UNORM;
		ao_tex_desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::RenderTarget;
		ao_texture = std::make_unique<GfxTexture>(gfx, ao_tex_desc);
	}
	void Renderer::CreateRenderPasses(Uint32 width, Uint32 height)
	{
		static constexpr Float clear_black[4] = {0.0f,0.0f,0.0f,0.0f};

		GfxColorAttachmentDesc gbuffer_normal_attachment{};
		gbuffer_normal_attachment.view = gbuffer[GBufferSlot_NormalMetallic]->RTV();
		gbuffer_normal_attachment.clear_color = GfxClearValue(clear_black);
		gbuffer_normal_attachment.load_op = GfxLoadAccessOp::Clear;

		GfxColorAttachmentDesc gbuffer_albedo_attachment{};
		gbuffer_albedo_attachment.view = gbuffer[GBufferSlot_DiffuseRoughness]->RTV();
		gbuffer_albedo_attachment.clear_color = GfxClearValue(clear_black);
		gbuffer_albedo_attachment.load_op = GfxLoadAccessOp::Clear;

		GfxColorAttachmentDesc gbuffer_emissive_attachment{};
		gbuffer_emissive_attachment.view = gbuffer[GBufferSlot_Emissive]->RTV();
		gbuffer_emissive_attachment.clear_color = GfxClearValue(clear_black);
		gbuffer_emissive_attachment.load_op = GfxLoadAccessOp::Clear;

		GfxColorAttachmentDesc decal_normal_attachment{};
		decal_normal_attachment.view = gbuffer[GBufferSlot_NormalMetallic]->RTV();
		decal_normal_attachment.load_op = GfxLoadAccessOp::Load;

		GfxColorAttachmentDesc decal_albedo_attachment{};
		decal_albedo_attachment.view = gbuffer[GBufferSlot_DiffuseRoughness]->RTV();
		decal_albedo_attachment.load_op = GfxLoadAccessOp::Load;

		GfxDepthAttachmentDesc depth_clear_attachment{};
		depth_clear_attachment.view = depth_target->DSV();
		depth_clear_attachment.clear_depth = GfxClearValue(1.0f, 0);
		depth_clear_attachment.load_op = GfxLoadAccessOp::Clear;

		GfxColorAttachmentDesc hdr_color_clear_attachment{};
		hdr_color_clear_attachment.view = hdr_render_target->RTV();
		hdr_color_clear_attachment.clear_color = GfxClearValue(clear_black);
		hdr_color_clear_attachment.load_op = GfxLoadAccessOp::Clear;

		GfxColorAttachmentDesc hdr_color_load_attachment{};
		hdr_color_load_attachment.view = hdr_render_target->RTV();
		hdr_color_load_attachment.load_op = GfxLoadAccessOp::Load;

		
		GfxDepthAttachmentDesc depth_load_attachment{};
		depth_load_attachment.view = depth_target->DSV();
		depth_load_attachment.clear_depth = GfxClearValue(clear_black);
		depth_load_attachment.load_op = GfxLoadAccessOp::Load;

		GfxColorAttachmentDesc fxaa_source_attachment{};
		fxaa_source_attachment.view = fxaa_texture->RTV();
		fxaa_source_attachment.load_op = GfxLoadAccessOp::DontCare;

		GfxDepthAttachmentDesc shadow_map_attachment{};
		shadow_map_attachment.view = shadow_depth_map->DSV();
		shadow_map_attachment.clear_depth = GfxClearValue(clear_black);
		shadow_map_attachment.load_op = GfxLoadAccessOp::Clear;

		GfxColorAttachmentDesc ssao_attachment{};
		ssao_attachment.view = ao_texture->RTV();
		ssao_attachment.load_op = GfxLoadAccessOp::DontCare;

		GfxColorAttachmentDesc ping_color_load_attachment{};
		ping_color_load_attachment.view = postprocess_textures[0]->RTV();
		ping_color_load_attachment.load_op = GfxLoadAccessOp::Load;

		GfxColorAttachmentDesc pong_color_load_attachment{};
		pong_color_load_attachment.view = postprocess_textures[1]->RTV();
		pong_color_load_attachment.load_op = GfxLoadAccessOp::Load;

		GfxColorAttachmentDesc offscreen_clear_attachment{};
		offscreen_clear_attachment.view = offscreen_ldr_render_target->RTV();
		offscreen_clear_attachment.clear_color = GfxClearValue(clear_black);
		offscreen_clear_attachment.load_op = GfxLoadAccessOp::Clear;

		GfxColorAttachmentDesc velocity_clear_attachment{};
		velocity_clear_attachment.view = velocity_buffer->RTV();
		velocity_clear_attachment.clear_color = GfxClearValue(clear_black);
		velocity_clear_attachment.load_op = GfxLoadAccessOp::Clear;

		//gbuffer pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(gbuffer_normal_attachment);
			render_pass_desc.rtv_attachments.push_back(gbuffer_albedo_attachment);
			render_pass_desc.rtv_attachments.push_back(gbuffer_emissive_attachment);
			render_pass_desc.dsv_attachment = depth_clear_attachment;
			gbuffer_pass = render_pass_desc;
		}

		//ambient pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(hdr_color_clear_attachment);
			ambient_pass = render_pass_desc;
		}

		//lighting pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(hdr_color_load_attachment);

			lighting_pass = render_pass_desc;
		}

		//forward pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(hdr_color_load_attachment);
			render_pass_desc.dsv_attachment = depth_load_attachment;
			forward_pass = render_pass_desc;
		}

		//particle pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(hdr_color_load_attachment);
			particle_pass = render_pass_desc;
		}

		//decal pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(decal_albedo_attachment);
			render_pass_desc.rtv_attachments.push_back(decal_normal_attachment);
			decal_pass = render_pass_desc;
		}

		//fxaa pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(fxaa_source_attachment);
			fxaa_pass = render_pass_desc;
		}

		//shadow map pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = SHADOW_MAP_SIZE;
			render_pass_desc.height = SHADOW_MAP_SIZE;
			render_pass_desc.dsv_attachment = shadow_map_attachment;
			shadow_map_pass = render_pass_desc;
		}

		//shadow cubemap pass
		{
			for (Uint32 i = 0; i < 6; ++i)
			{
				GfxDepthAttachmentDesc shadow_cubemap_attachment{};
				shadow_cubemap_attachment.view = shadow_depth_cubemap->DSV(i + 1);
				shadow_cubemap_attachment.clear_depth = GfxClearValue(1.0f, 0);
				shadow_cubemap_attachment.load_op = GfxLoadAccessOp::Clear;

				GfxRenderPassDesc render_pass_desc{};
				render_pass_desc.width = SHADOW_CUBE_SIZE;
				render_pass_desc.height = SHADOW_CUBE_SIZE;
				render_pass_desc.dsv_attachment = shadow_cubemap_attachment;
				shadow_cubemap_pass[i] = render_pass_desc;
			}
		}

		//cascade shadow pass
		{
			cascade_shadow_pass.clear();
			for (Uint32 i = 0; i < CASCADE_COUNT; ++i)
			{
				GfxDepthAttachmentDesc cascade_shadow_map_attachment{};
				cascade_shadow_map_attachment.view = shadow_cascade_maps->DSV(i + 1);
				cascade_shadow_map_attachment.clear_depth = GfxClearValue(1.0f, 0);
				cascade_shadow_map_attachment.load_op = GfxLoadAccessOp::Clear;

				GfxRenderPassDesc render_pass_desc{};
				render_pass_desc.width = SHADOW_CASCADE_SIZE;
				render_pass_desc.height = SHADOW_CASCADE_SIZE;
				render_pass_desc.dsv_attachment = cascade_shadow_map_attachment;
				cascade_shadow_pass.push_back(render_pass_desc);
			}
		}

		//ssao pass
		{

			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(ssao_attachment);
			ssao_pass = render_pass_desc;
			hbao_pass = ssao_pass;
		}

		//ping pong passes
		{
			
			GfxRenderPassDesc ping_render_pass_desc{};
			ping_render_pass_desc.width = width;
			ping_render_pass_desc.height = height;
			ping_render_pass_desc.rtv_attachments.push_back(ping_color_load_attachment);
			postprocess_passes[0] = ping_render_pass_desc;

			GfxRenderPassDesc pong_render_pass_desc{};
			pong_render_pass_desc.width = width;
			pong_render_pass_desc.height = height;
			pong_render_pass_desc.rtv_attachments.push_back(pong_color_load_attachment);
			postprocess_passes[1] = pong_render_pass_desc;

		}

		//voxel debug pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(hdr_color_clear_attachment);
			render_pass_desc.dsv_attachment = depth_load_attachment;
			voxel_debug_pass = render_pass_desc;
		}

		//offscreen_resolve_pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(offscreen_clear_attachment);
			offscreen_resolve_pass = render_pass_desc;
		}

		//velocity buffer pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(velocity_clear_attachment);
			velocity_buffer_pass = render_pass_desc;
		}
	}
	void Renderer::CreateComputeTextures(Uint32 width, Uint32 height)
	{
		GfxTextureDesc desc{};
		desc.width = width;
		desc.height = height;
		desc.format = GfxFormat::R16G16B16A16_FLOAT;
		desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::UnorderedAccess;
		
		blur_texture_intermediate = std::make_unique<GfxTexture>(gfx, desc);
		blur_texture_final = std::make_unique<GfxTexture>(gfx, desc);
		desc.misc_flags = GfxTextureMiscFlag::GenerateMips;
		desc.bind_flags |= GfxBindFlag::RenderTarget;
		bloom_extract_texture = std::make_unique<GfxTexture>(gfx, desc);
	}
	void Renderer::CreateIBLTextures()
	{
		//#TODO refactor IBL
		ibl_textures_generated = false;
	}

	void Renderer::BindGlobals()
	{
		static Bool called = false;

		if (!called)
		{
			GfxCommandContext* command_context = gfx->GetCommandContext();
			
			frame_cbuffer->Bind(command_context,  GfxShaderStage::VS, CBUFFER_SLOT_FRAME);
			object_cbuffer->Bind(command_context, GfxShaderStage::VS, CBUFFER_SLOT_OBJECT);
			shadow_cbuffer->Bind(command_context, GfxShaderStage::VS, CBUFFER_SLOT_SHADOW);
			weather_cbuffer->Bind(command_context, GfxShaderStage::VS, CBUFFER_SLOT_WEATHER);
			voxel_cbuffer->Bind(command_context,  GfxShaderStage::VS, CBUFFER_SLOT_VOXEL);

			command_context->SetSampler(GfxShaderStage::VS, 0, linear_wrap_sampler.get());
			
			//TS/HS GLOBALS
			frame_cbuffer->Bind(command_context, GfxShaderStage::DS, CBUFFER_SLOT_FRAME);
			frame_cbuffer->Bind(command_context, GfxShaderStage::HS, CBUFFER_SLOT_FRAME);

			command_context->SetSampler(GfxShaderStage::DS, 0, linear_wrap_sampler.get());
			command_context->SetSampler(GfxShaderStage::HS, 0, linear_wrap_sampler.get());

			//CS GLOBALS
			frame_cbuffer->Bind(command_context, GfxShaderStage::CS, CBUFFER_SLOT_FRAME);
			compute_cbuffer->Bind(command_context, GfxShaderStage::CS, CBUFFER_SLOT_COMPUTE);
			voxel_cbuffer->Bind(command_context, GfxShaderStage::CS, CBUFFER_SLOT_VOXEL);

			command_context->SetSampler(GfxShaderStage::CS, 0, linear_wrap_sampler.get());
			command_context->SetSampler(GfxShaderStage::CS, 1, point_wrap_sampler.get());
			command_context->SetSampler(GfxShaderStage::CS, 2, linear_border_sampler.get());
			command_context->SetSampler(GfxShaderStage::CS, 3, linear_clamp_sampler.get());

			//GS GLOBALS
			frame_cbuffer->Bind(command_context, GfxShaderStage::GS, CBUFFER_SLOT_FRAME);
			light_cbuffer->Bind(command_context, GfxShaderStage::GS, CBUFFER_SLOT_LIGHT);
			voxel_cbuffer->Bind(command_context, GfxShaderStage::GS, CBUFFER_SLOT_VOXEL);

			command_context->SetSampler(GfxShaderStage::GS, 1, point_wrap_sampler.get());
			command_context->SetSampler(GfxShaderStage::GS, 4, point_clamp_sampler.get());

			//PS GLOBALS
			material_cbuffer->Bind(command_context, GfxShaderStage::PS, CBUFFER_SLOT_MATERIAL);
			frame_cbuffer->Bind(command_context, GfxShaderStage::PS, CBUFFER_SLOT_FRAME);
			light_cbuffer->Bind(command_context, GfxShaderStage::PS, CBUFFER_SLOT_LIGHT);
			shadow_cbuffer->Bind(command_context, GfxShaderStage::PS, CBUFFER_SLOT_SHADOW);
			postprocess_cbuffer->Bind(command_context, GfxShaderStage::PS, CBUFFER_SLOT_POSTPROCESS);
			weather_cbuffer->Bind(command_context, GfxShaderStage::PS, CBUFFER_SLOT_WEATHER);
			voxel_cbuffer->Bind(command_context, GfxShaderStage::PS, CBUFFER_SLOT_VOXEL);
			terrain_cbuffer->Bind(command_context, GfxShaderStage::PS, CBUFFER_SLOT_TERRAIN);

			command_context->SetSampler(GfxShaderStage::PS, 0, linear_wrap_sampler.get());
			command_context->SetSampler(GfxShaderStage::PS, 1, point_wrap_sampler.get());
			command_context->SetSampler(GfxShaderStage::PS, 2, linear_border_sampler.get());
			command_context->SetSampler(GfxShaderStage::PS, 3, linear_clamp_sampler.get());
			command_context->SetSampler(GfxShaderStage::PS, 4, point_clamp_sampler.get());
			command_context->SetSampler(GfxShaderStage::PS, 5, shadow_sampler.get());
			command_context->SetSampler(GfxShaderStage::PS, 6, anisotropic_sampler.get());

			called = true;
		}

	}

	void Renderer::UpdateCBuffers(Float dt)
	{
		compute_cbuf_data.bokeh_blur_threshold = renderer_settings.bokeh_blur_threshold;
		compute_cbuf_data.bokeh_lum_threshold = renderer_settings.bokeh_lum_threshold;
		compute_cbuf_data.dof_params = Vector4(renderer_settings.dof_near_blur, renderer_settings.dof_near, renderer_settings.dof_far, renderer_settings.dof_far_blur);
		compute_cbuf_data.bokeh_radius_scale = renderer_settings.bokeh_radius_scale;
		compute_cbuf_data.bokeh_color_scale = renderer_settings.bokeh_color_scale;
		compute_cbuf_data.bokeh_fallout = renderer_settings.bokeh_fallout;

		compute_cbuf_data.visualize_tiled = static_cast<Sint32>(renderer_settings.visualize_tiled);
		compute_cbuf_data.visualize_max_lights = renderer_settings.visualize_max_lights;

		compute_cbuf_data.threshold = renderer_settings.bloom_threshold;
		compute_cbuf_data.bloom_scale = renderer_settings.bloom_scale;

		std::array<Float, 9> coeffs{};
		coeffs.fill(1.0f / 9);
		compute_cbuf_data.gauss_coeff1 = coeffs[0];
		compute_cbuf_data.gauss_coeff2 = coeffs[1];
		compute_cbuf_data.gauss_coeff3 = coeffs[2];
		compute_cbuf_data.gauss_coeff4 = coeffs[3];
		compute_cbuf_data.gauss_coeff5 = coeffs[4];
		compute_cbuf_data.gauss_coeff6 = coeffs[5];
		compute_cbuf_data.gauss_coeff7 = coeffs[6];
		compute_cbuf_data.gauss_coeff8 = coeffs[7];
		compute_cbuf_data.gauss_coeff9 = coeffs[8];

		compute_cbuf_data.ocean_choppiness = renderer_settings.ocean_choppiness;
		compute_cbuf_data.ocean_size = 512;
		compute_cbuf_data.resolution = RESOLUTION;
		compute_cbuf_data.wind_direction_x = renderer_settings.wind_direction[0];
		compute_cbuf_data.wind_direction_y = renderer_settings.wind_direction[1];
		compute_cbuf_data.delta_time = dt;

		compute_cbuffer->Update(gfx->GetCommandContext(), compute_cbuf_data);
	}
	void Renderer::UpdateOcean(Float dt)
	{
		if (reg.size<Ocean>() == 0) return;
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Ocean Update Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Ocean Update Pass");

		if (renderer_settings.ocean_color_changed)
		{
			auto ocean_view = reg.view<Ocean, Material>();
			for (auto e : ocean_view)
			{
				auto& material = ocean_view.get<Material>(e);
				material.diffuse = Vector3(renderer_settings.ocean_color);
			}
		}

		if (renderer_settings.recreate_initial_spectrum)
		{
			ShaderManager::GetShaderProgram(ShaderProgram::OceanInitialSpectrum)->Bind(command_context);
			GfxShaderResourceRW uav[] = { ocean_initial_spectrum->UAV() };
			command_context->SetShaderResourcesRW(0, uav);
			command_context->Dispatch(RESOLUTION / 32, RESOLUTION / 32, 1);
			command_context->UnsetShaderResourcesRW(0, 1);
			renderer_settings.recreate_initial_spectrum = false;
		}

		//phase
		{
			GfxShaderResourceRO  srv[] = { ping_pong_phase_textures[pong_phase]->SRV()};
			GfxShaderResourceRW uav[] = { ping_pong_phase_textures[!pong_phase]->UAV() };

			ShaderManager::GetShaderProgram(ShaderProgram::OceanPhase)->Bind(command_context);
			command_context->SetShaderResourcesRO(GfxShaderStage::CS, 0, srv);
			command_context->SetShaderResourcesRW(0, uav);
			command_context->Dispatch(RESOLUTION / 32, RESOLUTION / 32, 1);
			command_context->UnsetShaderResourcesRO(GfxShaderStage::CS, 0, 1);
			command_context->UnsetShaderResourcesRW(0, 1);
			pong_phase = !pong_phase;
		}

		//spectrum
		{
			GfxShaderResourceRO  srvs[]	= { ping_pong_phase_textures[pong_phase]->SRV(), ocean_initial_spectrum->SRV() };
			GfxShaderResourceRW  uav[]	= { ping_pong_spectrum_textures[pong_spectrum]->UAV() };
			ShaderManager::GetShaderProgram(ShaderProgram::OceanSpectrum)->Bind(command_context);
			command_context->SetShaderResourcesRO(GfxShaderStage::CS, 0, srvs);
			command_context->SetShaderResourcesRW(0, uav);
			command_context->Dispatch(RESOLUTION / 32, RESOLUTION / 32, 1);
			command_context->UnsetShaderResourcesRO(GfxShaderStage::CS, 0, 2);
			command_context->UnsetShaderResourcesRW(0, 1);
			pong_spectrum = !pong_spectrum;
		}

		//fft
		{
			struct FFTCBuffer
			{
				Uint32 seq_count;
				Uint32 subseq_count;
			};
			static FFTCBuffer fft_cbuf_data{ .seq_count = RESOLUTION };
			static GfxConstantBuffer<FFTCBuffer> fft_cbuffer(gfx);

			fft_cbuffer.Bind(command_context, GfxShaderStage::CS, 10);
			{
				ShaderManager::GetShaderProgram(ShaderProgram::OceanFFT_Horizontal)->Bind(command_context);
				for (Uint32 p = 1; p < RESOLUTION; p <<= 1)
				{

					GfxShaderResourceRO  srv[] = { ping_pong_spectrum_textures[!pong_spectrum]->SRV()};
					GfxShaderResourceRW uav[] = { ping_pong_spectrum_textures[pong_spectrum]->UAV() };

					fft_cbuf_data.subseq_count = p;
					fft_cbuffer.Update(command_context, fft_cbuf_data);

					command_context->SetShaderResourcesRO(GfxShaderStage::CS, 0, srv);
					command_context->SetShaderResourcesRW(0, uav);
					command_context->Dispatch(RESOLUTION, 1, 1);
					command_context->UnsetShaderResourcesRO(GfxShaderStage::CS, 0, 1);
					command_context->UnsetShaderResourcesRW(0, 1);
					pong_spectrum = !pong_spectrum;
				}
			}
			//fft vertical
			{
				ShaderManager::GetShaderProgram(ShaderProgram::OceanFFT_Vertical)->Bind(command_context);
				for (Uint32 p = 1; p < RESOLUTION; p <<= 1)
				{
					GfxShaderResourceRO  srv[]  = { ping_pong_spectrum_textures[!pong_spectrum]->SRV() };
					GfxShaderResourceRW uav[]  = { ping_pong_spectrum_textures[pong_spectrum]->UAV() };

					fft_cbuf_data.subseq_count = p;
					fft_cbuffer.Update(command_context, fft_cbuf_data);

					command_context->SetShaderResourcesRO(GfxShaderStage::CS, 0, srv);
					command_context->SetShaderResourcesRW(0, uav);
					command_context->Dispatch(RESOLUTION, 1, 1);
					command_context->UnsetShaderResourcesRO(GfxShaderStage::CS, 0, 1);
					command_context->UnsetShaderResourcesRW(0, 1);
					pong_spectrum = !pong_spectrum;
				}
			}
		}
		//normal
		{
			ShaderManager::GetShaderProgram(ShaderProgram::OceanNormalMap)->Bind(command_context);

			GfxShaderResourceRO  final_spectrum = ping_pong_spectrum_textures[!pong_spectrum]->SRV();
			GfxShaderResourceRW normal_map_uav = ocean_normal_map->UAV();

			command_context->SetShaderResourceRO(GfxShaderStage::CS, 0, final_spectrum);
			command_context->SetShaderResourceRW(0, normal_map_uav);
			command_context->Dispatch(RESOLUTION / 32, RESOLUTION / 32, 1);

			command_context->SetShaderResourceRO(GfxShaderStage::CS, 0, nullptr);
			command_context->SetShaderResourceRW(0, nullptr);
		}
	}
	void Renderer::UpdateWeather(Float dt)
	{
		static Float total_time = 0.0f;
		total_time += dt;

		auto lights = reg.view<Light>();

		for (auto light : lights)
		{
			auto const& light_data = lights.get(light);

			if (light_data.type == LightType::Directional && light_data.active)
			{
				weather_cbuf_data.light_dir = -light_data.direction; weather_cbuf_data.light_dir.Normalize();
				weather_cbuf_data.light_color = light_data.color * light_data.energy;
				break;
			}
		}

		weather_cbuf_data.sky_color = Vector4{ renderer_settings.sky_color[0], renderer_settings.sky_color[1],renderer_settings.sky_color[2], 1.0f };
		weather_cbuf_data.ambient_color = Vector4{ renderer_settings.ambient_color[0], renderer_settings.ambient_color[1],renderer_settings.ambient_color[2], 1.0f };
		weather_cbuf_data.wind_dir = Vector4{ renderer_settings.wind_direction[0], 0.0f, renderer_settings.wind_direction[1], 0.0f };
		weather_cbuf_data.wind_speed = renderer_settings.wind_speed;
		weather_cbuf_data.time = total_time;
		weather_cbuf_data.crispiness	= renderer_settings.crispiness;
		weather_cbuf_data.curliness		= renderer_settings.curliness;
		weather_cbuf_data.coverage		= renderer_settings.coverage;
		weather_cbuf_data.absorption	= renderer_settings.light_absorption;
		weather_cbuf_data.clouds_bottom_height = renderer_settings.clouds_bottom_height;
		weather_cbuf_data.clouds_top_height = renderer_settings.clouds_top_height;
		weather_cbuf_data.density_factor = renderer_settings.density_factor;
		weather_cbuf_data.cloud_type = renderer_settings.cloud_type;

		Vector3 sun_dir = Vector3(&weather_cbuf_data.light_dir.x); sun_dir.Normalize();
		SkyParameters sky_params = CalculateSkyParameters(renderer_settings.turbidity, renderer_settings.ground_albedo, sun_dir);

		weather_cbuf_data.A = sky_params[(size_t)ESkyParam_A];
		weather_cbuf_data.B = sky_params[(size_t)ESkyParam_B];
		weather_cbuf_data.C = sky_params[(size_t)ESkyParam_C];
		weather_cbuf_data.D = sky_params[(size_t)ESkyParam_D];
		weather_cbuf_data.E = sky_params[(size_t)ESkyParam_E];
		weather_cbuf_data.F = sky_params[(size_t)ESkyParam_F];
		weather_cbuf_data.G = sky_params[(size_t)ESkyParam_G];
		weather_cbuf_data.H = sky_params[(size_t)ESkyParam_H];
		weather_cbuf_data.I = sky_params[(size_t)ESkyParam_I];
		weather_cbuf_data.Z = sky_params[(size_t)ESkyParam_Z];

		weather_cbuffer->Update(gfx->GetCommandContext(), weather_cbuf_data);
	}
	void Renderer::UpdateParticles(Float dt)
	{
		auto emitters = reg.view<Emitter>();
		for (auto emitter : emitters)
		{
			Emitter& emitter_params = emitters.get(emitter);
			particle_renderer.Update(dt, emitter_params);
		}
	}

	void Renderer::UpdateLights()
	{
		Uint32 current_light_count = (Uint32)reg.size<Light>();
		static Uint32 light_count = 0;
		Bool light_count_changed = current_light_count != light_count;
		if (light_count_changed)
		{
			light_count = current_light_count;
			if (light_count == 0) return;
			lights = std::make_unique<GfxBuffer>(gfx, StructuredBufferDesc<LightSBuffer>(light_count, false, true));
			lights->CreateSRV();
		}

		std::vector<LightSBuffer> lights_data{};

		auto light_view = reg.view<Light>();
		for (auto e : light_view)
		{
			LightSBuffer light_data{};
			auto& light = light_view.get(e);

			light_data.color = light.color * light.energy;
			light_data.position  = Vector4::Transform(light.position, camera->View());
			light_data.direction = Vector4::Transform(light.direction, camera->View());
			light_data.range = light.range;
			light_data.type = static_cast<Sint32>(light.type);
			light_data.inner_cosine = light.inner_cosine;
			light_data.outer_cosine = light.outer_cosine;
			light_data.active = light.active;
			light_data.casts_shadows = light.casts_shadows;
			
			lights_data.push_back(light_data);
		}
		lights->Update(lights_data.data(), lights_data.size() * sizeof(LightSBuffer));

	}
	void Renderer::UpdateTerrainData()
	{
		terrain_cbuf_data.texture_scale = TerrainComponent::texture_scale;
		terrain_cbuf_data.ocean_active = reg.size<Ocean>() != 0;
		terrain_cbuffer->Update(gfx->GetCommandContext(), terrain_cbuf_data);
	}
	void Renderer::UpdateVoxelData()
	{
		Float const f = 0.05f / renderer_settings.voxel_size;

		Vector3 cam_pos = camera->Position();
		Vector3 center(floorf(cam_pos.x * f) / f, floorf(cam_pos.y * f) / f, floorf(cam_pos.z * f) / f);

		Vector3 voxel_center(renderer_settings.voxel_center_x, renderer_settings.voxel_center_y, renderer_settings.voxel_center_z);
		if(Vector3::DistanceSquared(voxel_center, center) > 0.01f)
		{
			renderer_settings.voxel_center_x = center.x;
			renderer_settings.voxel_center_y = center.y;
			renderer_settings.voxel_center_z = center.z;
		}

		voxel_cbuf_data.data_res = VOXEL_RESOLUTION;
		voxel_cbuf_data.data_res_rcp = 1.0f / VOXEL_RESOLUTION;
		voxel_cbuf_data.data_size = renderer_settings.voxel_size;
		voxel_cbuf_data.data_size_rcp = 1.0f / renderer_settings.voxel_size;
		voxel_cbuf_data.mips = 7;
		voxel_cbuf_data.num_cones = renderer_settings.voxel_num_cones;
		voxel_cbuf_data.num_cones_rcp = 1.0f / renderer_settings.voxel_num_cones;
		voxel_cbuf_data.ray_step_size = renderer_settings.voxel_ray_step_distance;
		voxel_cbuf_data.max_distance = renderer_settings.voxel_max_distance;
		voxel_cbuf_data.grid_center = center;

		voxel_cbuffer->Update(gfx->GetCommandContext(), voxel_cbuf_data);
	}
	void Renderer::CameraFrustumCulling()
	{
		BoundingFrustum camera_frustum = camera->Frustum();
		auto aabb_view = reg.view<AABB>();
		for (auto e : aabb_view)
		{
			auto& aabb = aabb_view.get(e);
			if (aabb.skip_culling) continue;
			aabb.camera_visible = camera_frustum.Intersects(aabb.bounding_box) || reg.has<Light>(e); //dont cull lights for now
		}
	}
	void Renderer::LightFrustumCulling(LightType type)
	{
		auto visibility_view = reg.view<AABB>();
		for (auto e : visibility_view)
		{
			if (reg.has<Light>(e)) continue; 
			auto& aabb = visibility_view.get(e);
			if (aabb.skip_culling) continue;

			switch (type)
			{
			case LightType::Directional:
				aabb.light_visible = light_bounding_box.Intersects(aabb.bounding_box);
				break;
			case LightType::Spot:
			case LightType::Point:
				aabb.light_visible = light_bounding_frustum.Intersects(aabb.bounding_box);
				break;
			default:
				ADRIA_ASSERT(false);
			}
		}
	}

	void Renderer::PassPicking()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxScopedAnnotation(command_context, "Picking Pass");
		ADRIA_ASSERT(pick_in_current_frame);
		pick_in_current_frame = false;
		last_picking_data = picker.Pick(depth_target->SRV(), gbuffer[GBufferSlot_NormalMetallic]->SRV());
	}
	void Renderer::PassGBuffer()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "GBuffer Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "GBuffer Pass");

		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, (Uint32)gbuffer.size() + 1);
		struct BatchParams
		{
			ShaderProgram shader_program;
			Bool double_sided;
			auto operator<=>(BatchParams const&) const = default;
		};
		std::map<BatchParams, std::vector<entity>> batched_entities;

		auto gbuffer_view = reg.view<Mesh, Transform, Material, Deferred, AABB>();
		for (auto e : gbuffer_view)
		{
			auto [mesh, transform, material, aabb] = gbuffer_view.get<Mesh, Transform, Material, AABB>(e);
			if (!aabb.camera_visible) continue;

			BatchParams params{};
			params.double_sided = material.double_sided;
			params.shader_program = material.alpha_mode == MaterialAlphaMode::Opaque ? ShaderProgram::GBufferPBR : ShaderProgram::GBufferPBR_Mask;
			batched_entities[params].push_back(e);
		}
		
		command_context->BeginRenderPass(gbuffer_pass);
		{
			for (auto const& [params, entities] : batched_entities)
			{
				ShaderManager::GetShaderProgram(params.shader_program)->Bind(command_context);
				if (params.double_sided) command_context->SetRasterizerState(cull_none.get()); 
				for (auto e : entities)
				{
					auto [mesh, transform, material] = gbuffer_view.get<Mesh, Transform, Material>(e);

					Matrix parent_transform = Matrix::Identity;
					if (Relationship* relationship = reg.get_if<Relationship>(e))
					{
						if (auto* root_transform = reg.get_if<Transform>(relationship->parent)) parent_transform = root_transform->current_transform;
					}
					
					object_cbuf_data.model = transform.current_transform * parent_transform;
					object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
					object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);

					material_cbuf_data.albedo_factor = material.albedo_factor;
					material_cbuf_data.metallic_factor = material.metallic_factor;
					material_cbuf_data.roughness_factor = material.roughness_factor;
					material_cbuf_data.emissive_factor = material.emissive_factor;
					material_cbuf_data.alpha_cutoff = material.alpha_cutoff;
					material_cbuffer->Update(gfx->GetCommandContext(), material_cbuf_data);

					static GfxShaderResourceRO const null_view = nullptr;

					if (material.albedo_texture != INVALID_TEXTURE_HANDLE)
					{
						auto view = g_TextureManager.GetTextureView(material.albedo_texture);
						command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_DIFFUSE, view);
					}

					if (material.metallic_roughness_texture != INVALID_TEXTURE_HANDLE)
					{
						auto view = g_TextureManager.GetTextureView(material.metallic_roughness_texture);
						command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_ROUGHNESS_METALLIC, view);
					}
					else
					{
						command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_ROUGHNESS_METALLIC, nullptr);
					}

					if (material.normal_texture != INVALID_TEXTURE_HANDLE)
					{
						auto view = g_TextureManager.GetTextureView(material.normal_texture);
						command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_NORMAL, view);

					}
					else
					{
						command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_NORMAL, nullptr);
					}

					if (material.emissive_texture != INVALID_TEXTURE_HANDLE)
					{
						auto view = g_TextureManager.GetTextureView(material.emissive_texture);
						command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_EMISSIVE, view);
					}
					else
					{
						command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_EMISSIVE, nullptr);
					}

					mesh.Draw(command_context);
				}
				if (params.double_sided) command_context->SetRasterizerState(nullptr);
			}
			
			auto terrain_view = reg.view<Mesh, Transform, AABB, TerrainComponent>();
			ShaderManager::GetShaderProgram(ShaderProgram::GBuffer_Terrain)->Bind(command_context);
			for (auto e : terrain_view)
			{
				auto [mesh, transform, aabb, terrain] = terrain_view.get<Mesh, Transform, AABB, TerrainComponent>(e);

				if (!aabb.camera_visible) continue;

				object_cbuf_data.model = transform.current_transform;
				object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert().Transpose();
				object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);

				if (terrain.grass_texture != INVALID_TEXTURE_HANDLE)
				{
					auto view = g_TextureManager.GetTextureView(terrain.grass_texture);
					command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_GRASS, view);
				}
				if (terrain.base_texture != INVALID_TEXTURE_HANDLE)
				{
					auto view = g_TextureManager.GetTextureView(terrain.base_texture);
					command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_BASE, view);
				}
				if (terrain.rock_texture != INVALID_TEXTURE_HANDLE)
				{
					auto view = g_TextureManager.GetTextureView(terrain.rock_texture);
					command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_ROCK, view);
				}
				if (terrain.sand_texture != INVALID_TEXTURE_HANDLE)
				{
					auto view = g_TextureManager.GetTextureView(terrain.sand_texture);
					command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_SAND, view);
				}

				if (terrain.layer_texture != INVALID_TEXTURE_HANDLE)
				{
					auto view = g_TextureManager.GetTextureView(terrain.layer_texture);
					command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_LAYER, view);
				}
				mesh.Draw(command_context);
			}

			auto foliage_view = reg.view<Mesh, Transform, Material, AABB, Foliage>();
			ShaderManager::GetShaderProgram(ShaderProgram::GBuffer_Foliage)->Bind(command_context);
			for (auto e : foliage_view)
			{
				auto [mesh, transform, aabb, material] = foliage_view.get<Mesh, Transform, AABB, Material>(e);
				if (!aabb.camera_visible) continue;

				object_cbuf_data.model = transform.current_transform;
				object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert().Transpose();
				object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);

				if (material.albedo_texture != INVALID_TEXTURE_HANDLE)
				{
					auto view = g_TextureManager.GetTextureView(material.albedo_texture);
					command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_DIFFUSE, view);
				}
				mesh.Draw(command_context);
			}
		}
		command_context->EndRenderPass();
	}
	void Renderer::PassDecals()
	{
		if (reg.size<Decal>() == 0) return;
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Decals Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Decals Pass");

		struct DecalCBuffer
		{
			Sint32 decal_type;
			Bool32 modify_gbuffer_normals;
		};

		static GfxConstantBuffer<DecalCBuffer> decal_cbuffer(gfx);
		static DecalCBuffer decal_cbuf_data{};
		decal_cbuffer.Bind(command_context, GfxShaderStage::PS, 11);

		command_context->BeginRenderPass(decal_pass);
		{
			command_context->SetTopology(GfxPrimitiveTopology::TriangleList);
			command_context->SetVertexBuffer(cube_vb.get());
			command_context->SetIndexBuffer(cube_ib.get());
			auto decal_view = reg.view<Decal>();

			command_context->SetRasterizerState(cull_none.get());
			for (auto e : decal_view)
			{
				Decal decal = decal_view.get(e);
				decal.modify_gbuffer_normals 
					? ShaderManager::GetShaderProgram(ShaderProgram::Decals_ModifyNormals)->Bind(command_context) 
					: ShaderManager::GetShaderProgram(ShaderProgram::Decals)->Bind(command_context);
				decal_cbuf_data.decal_type = static_cast<Sint32>(decal.decal_type);
				decal_cbuffer.Update(command_context, decal_cbuf_data);

				object_cbuf_data.model = decal.decal_model_matrix;
				object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert().Transpose();
				object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);

				GfxShaderResourceRO srvs[] = { g_TextureManager.GetTextureView(decal.albedo_decal_texture), 
												 g_TextureManager.GetTextureView(decal.normal_decal_texture),
												 depth_target->SRV() };
				command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srvs);
				command_context->DrawIndexed(cube_ib->GetCount());
			}
			command_context->SetRasterizerState(nullptr);
		}
		command_context->EndRenderPass();
	}

	void Renderer::PassSSAO()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "SSAO Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "SSAO Pass");

		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 7, 1);
		{
			for (Uint32 i = 0; i < ssao_kernel.size(); ++i) postprocess_cbuf_data.samples[i] = ssao_kernel[i];
			postprocess_cbuf_data.noise_scale = Vector2((Float)width / 8, (Float)height / 8);
			postprocess_cbuf_data.ssao_power = renderer_settings.ssao_power;
			postprocess_cbuf_data.ssao_radius = renderer_settings.ssao_radius;
			postprocess_cbuffer->Update(gfx->GetCommandContext(), postprocess_cbuf_data);
		}

		command_context->BeginRenderPass(ssao_pass);
		{
			GfxShaderResourceRO srvs[] = { gbuffer[GBufferSlot_NormalMetallic]->SRV(), depth_target->SRV(), ssao_random_texture->SRV() };
			command_context->SetShaderResourcesRO(GfxShaderStage::PS, 1, srvs);
			command_context->SetInputLayout(nullptr);
			command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
			ShaderManager::GetShaderProgram(ShaderProgram::SSAO)->Bind(command_context);
			command_context->Draw(4);
			command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 1, ARRAYSIZE(srvs));
		}
		command_context->EndRenderPass();
		BlurTexture(ao_texture.get());
		GfxShaderResourceRO blurred_ssao = blur_texture_final->SRV();
		command_context->SetShaderResourceRO(GfxShaderStage::PS, 7, blurred_ssao);
	}
	void Renderer::PassHBAO()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "HBAO Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "HBAO Pass");

		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 7, 1);
		{
			postprocess_cbuf_data.noise_scale = Vector2((Float)width / 8, (Float)height / 8);
			postprocess_cbuf_data.hbao_r2 = renderer_settings.hbao_radius * renderer_settings.hbao_radius;
			postprocess_cbuf_data.hbao_radius_to_screen = renderer_settings.hbao_radius * 0.5f * Float(height) / (tanf(camera->Fov() * 0.5f) * 2.0f);
			postprocess_cbuf_data.hbao_power = renderer_settings.hbao_power;
			postprocess_cbuffer->Update(gfx->GetCommandContext(), postprocess_cbuf_data);
		}

		command_context->BeginRenderPass(hbao_pass);
		{
			GfxShaderResourceRO srvs[] = { gbuffer[GBufferSlot_NormalMetallic]->SRV(), depth_target->SRV(), ssao_random_texture->SRV() };
			command_context->SetShaderResourcesRO(GfxShaderStage::PS, 1, srvs);
			command_context->SetInputLayout(nullptr);
			command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
			ShaderManager::GetShaderProgram(ShaderProgram::HBAO)->Bind(command_context);
			command_context->Draw(4);
			command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 1, ARRAYSIZE(srvs));
		}
		command_context->EndRenderPass();
		BlurTexture(ao_texture.get());

		GfxShaderResourceRO blurred_ssao = blur_texture_final->SRV();
		command_context->SetShaderResourceRO(GfxShaderStage::PS, 7, blurred_ssao);
	}
	void Renderer::PassAmbient()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Ambient Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Ambient Pass");

		GfxShaderResourceRO srvs[] = { gbuffer[GBufferSlot_NormalMetallic]->SRV(), gbuffer[GBufferSlot_DiffuseRoughness]->SRV(), depth_target->SRV(), gbuffer[GBufferSlot_Emissive]->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srvs);

		command_context->BeginRenderPass(ambient_pass);
		if (renderer_settings.ibl)
		{
			GfxShaderResourceRO ibl_srvs[] = { env_srv.Get(),irmap_srv.Get(), brdf_srv.Get() };
			command_context->SetShaderResourcesRO(GfxShaderStage::PS, 8, ibl_srvs);
		}

		command_context->SetInputLayout(nullptr);
		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);

		if (!ibl_textures_generated) renderer_settings.ibl = false;
		Bool has_ao = renderer_settings.ambient_occlusion != AmbientOcclusion::None;
		if (has_ao && renderer_settings.ibl) ShaderManager::GetShaderProgram(ShaderProgram::AmbientPBR_AO_IBL)->Bind(command_context);
		else if (has_ao && !renderer_settings.ibl) ShaderManager::GetShaderProgram(ShaderProgram::AmbientPBR_AO)->Bind(command_context);
		else if (!has_ao && renderer_settings.ibl) ShaderManager::GetShaderProgram(ShaderProgram::AmbientPBR_IBL)->Bind(command_context);
		else ShaderManager::GetShaderProgram(ShaderProgram::AmbientPBR)->Bind(command_context);
		command_context->Draw(4);

		command_context->EndRenderPass();

		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, 4);
		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 7, 4);
	}
	void Renderer::PassDeferredLighting()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Deferred Lighting Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Deferred Lighting Pass");

		command_context->SetBlendState(additive_blend.get());
		auto lights = reg.view<Light>();
		for (entity light : lights)
		{
			auto const& light_data = lights.get(light);

			if (!light_data.active) continue;
			if ((renderer_settings.use_tiled_deferred || renderer_settings.use_clustered_deferred) //tiled/clustered deferred takes care of noncasting lights
				&& !light_data.casts_shadows) continue; 

			//update cbuffer
			{
				light_cbuf_data.casts_shadows = light_data.casts_shadows;
				light_cbuf_data.color = light_data.color * light_data.energy;
				light_cbuf_data.direction = light_data.direction;
				light_cbuf_data.inner_cosine = light_data.inner_cosine;
				light_cbuf_data.outer_cosine = light_data.outer_cosine;
				light_cbuf_data.position = light_data.position;
				light_cbuf_data.range = light_data.range;
				light_cbuf_data.type = static_cast<Sint32>(light_data.type);
				light_cbuf_data.use_cascades = light_data.use_cascades;
				light_cbuf_data.volumetric_strength = light_data.volumetric_strength;
				light_cbuf_data.sscs = light_data.screen_space_contact_shadows;
				light_cbuf_data.sscs_thickness = light_data.sscs_thickness;
				light_cbuf_data.sscs_max_ray_distance = light_data.sscs_max_ray_distance;
				light_cbuf_data.sscs_max_depth_distance = light_data.sscs_max_depth_distance;
				
				Matrix camera_view = camera->View();
				light_cbuf_data.position = Vector4::Transform(light_cbuf_data.position, camera_view);
				light_cbuf_data.direction = Vector4::Transform(light_cbuf_data.direction, camera_view);
				light_cbuffer->Update(gfx->GetCommandContext(), light_cbuf_data);
			}

			//shadow mapping
			{
				if (light_data.casts_shadows)
				{
					switch (light_data.type)
					{
					case LightType::Directional:
						if (light_data.use_cascades) PassShadowMapCascades(light_data);
						else PassShadowMapDirectional(light_data);
						break;
					case LightType::Spot:
						PassShadowMapSpot(light_data);
						break;
					case LightType::Point:
						PassShadowMapPoint(light_data);
						break;
					default:
						ADRIA_ASSERT(false);
					}

				}
			}

			//lighting + volumetric fog
			command_context->BeginRenderPass(lighting_pass);
			{
				GfxShaderResourceRO shader_views[3] = { nullptr };
				shader_views[0] = gbuffer[GBufferSlot_NormalMetallic]->SRV();
				shader_views[1] = gbuffer[GBufferSlot_DiffuseRoughness]->SRV();
				shader_views[2] = depth_target->SRV();

				command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, shader_views);
				command_context->SetInputLayout(nullptr);
				command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
				ShaderManager::GetShaderProgram(ShaderProgram::LightingPBR)->Bind(command_context);
				command_context->Draw(4);
				command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, ARRAYSIZE(shader_views));
				if (light_data.volumetric) PassVolumetric(light_data);
			}
			command_context->EndRenderPass();
		}
		command_context->SetBlendState(nullptr);
	}
	void Renderer::PassDeferredTiledLighting()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Deferred Tiled Lighting Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Deferred Tiled Lighting Pass");

		GfxShaderResourceRO shader_views[3] = { nullptr };
		shader_views[0] = gbuffer[GBufferSlot_NormalMetallic]->SRV();
		shader_views[1] = gbuffer[GBufferSlot_DiffuseRoughness]->SRV();
		shader_views[2] = depth_target->SRV();
		command_context->SetShaderResourcesRO(GfxShaderStage::CS, 0, shader_views);

		GfxShaderResourceRO lights_srv = lights->SRV();
		command_context->SetShaderResourceRO(GfxShaderStage::CS, 3, lights_srv);
		GfxShaderResourceRW texture_uav = uav_target->UAV();
		command_context->SetShaderResourceRW(0, texture_uav);

		GfxShaderResourceRW debug_uav = debug_tiled_texture->UAV();
		if (renderer_settings.visualize_tiled)
		{
			command_context->SetShaderResourceRW(1, debug_uav);
		}

		ShaderManager::GetShaderProgram(ShaderProgram::TiledLighting)->Bind(command_context);
		command_context->Dispatch((Uint32)std::ceil(width * 1.0f / 16), (Uint32)std::ceil(height * 1.0f / 16), 1);

		command_context->UnsetShaderResourcesRO(GfxShaderStage::CS, 0, 4);
		command_context->UnsetShaderResourcesRW(0, 1);

		GfxRenderTarget rtv[] = { hdr_render_target->RTV() };
		command_context->SetRenderTargets(rtv, nullptr);
		if (renderer_settings.visualize_tiled)
		{
			command_context->UnsetShaderResourcesRW(1, 1);
			command_context->SetBlendState(alpha_blend.get());
			AddTextures(uav_target.get(), debug_tiled_texture.get());
			command_context->SetBlendState(nullptr);
		}
		else
		{
			command_context->SetBlendState(additive_blend.get());
			CopyTexture(uav_target.get());
			command_context->SetBlendState(nullptr);
		}
		
		Float black[4] = { 0.0f,0.0f,0.0f,0.0f };
		command_context->ClearReadWriteDescriptorFloat(debug_uav, black);
		command_context->ClearReadWriteDescriptorFloat(texture_uav, black);

		std::vector<Light> volumetric_lights{};

		auto light_view = reg.view<Light>();
		for (auto e : light_view)
		{
			auto const& light = light_view.get(e);
			if (light.volumetric) volumetric_lights.push_back(light);
		}

		//Volumetric lighting
		if (renderer_settings.visualize_tiled || volumetric_lights.empty()) return;
		command_context->BeginRenderPass(lighting_pass);
		command_context->SetBlendState(additive_blend.get());
		for (auto const& light : volumetric_lights)
		{
			ADRIA_ASSERT(light.volumetric);
			light_cbuf_data.casts_shadows = light.casts_shadows;
			light_cbuf_data.color = light.color * light.energy;
			light_cbuf_data.direction = light.direction;
			light_cbuf_data.inner_cosine = light.inner_cosine;
			light_cbuf_data.outer_cosine = light.outer_cosine;
			light_cbuf_data.position = light.position;
			light_cbuf_data.range = light.range;
			light_cbuf_data.volumetric_strength = light.volumetric_strength;
				
			Matrix camera_view = camera->View();
			light_cbuf_data.position = Vector4::Transform(light_cbuf_data.position, camera_view);
			light_cbuf_data.direction = Vector4::Transform(light_cbuf_data.direction, camera_view);
			light_cbuffer->Update(gfx->GetCommandContext(), light_cbuf_data);

			PassVolumetric(light);
		}
		command_context->SetBlendState(nullptr);
		command_context->EndRenderPass();
	}
	void Renderer::PassDeferredClusteredLighting()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Deferred Clustered Lighting Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Deferred Clustered Lighting Pass");

		if (recreate_clusters)
		{
			GfxShaderResourceRW clusters_uav = clusters->UAV();
			command_context->SetShaderResourceRW(0, clusters_uav);
			
			ShaderManager::GetShaderProgram(ShaderProgram::ClusterBuilding)->Bind(command_context);
			command_context->Dispatch(CLUSTER_SIZE_X, CLUSTER_SIZE_Y, CLUSTER_SIZE_Z);
			ShaderManager::GetShaderProgram(ShaderProgram::ClusterBuilding)->Unbind(command_context);

			GfxShaderResourceRW null_uav = nullptr;
			command_context->SetShaderResourceRW(0, nullptr);

			recreate_clusters = false;
		}

		GfxShaderResourceRO srvs[] = { clusters->SRV(), lights->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::CS, 0, srvs);
		GfxShaderResourceRW uavs[] = { light_counter->UAV(), light_list->UAV(), light_grid->UAV() };
		command_context->SetShaderResourcesRW(0, uavs);

		ShaderManager::GetShaderProgram(ShaderProgram::ClusterCulling)->Bind(command_context);
		command_context->Dispatch(CLUSTER_SIZE_X / 16, CLUSTER_SIZE_Y / 16, CLUSTER_SIZE_Z / 4);
		ShaderManager::GetShaderProgram(ShaderProgram::ClusterCulling)->Unbind(command_context);

		command_context->UnsetShaderResourcesRO(GfxShaderStage::CS, 0, ARRAYSIZE(srvs));
		command_context->UnsetShaderResourcesRW(0, ARRAYSIZE(uavs));

		command_context->SetBlendState(additive_blend.get());

		command_context->BeginRenderPass(lighting_pass);
		{
			GfxShaderResourceRO shader_views[6] = { nullptr };
			shader_views[0] = gbuffer[GBufferSlot_NormalMetallic]->SRV();
			shader_views[1] = gbuffer[GBufferSlot_DiffuseRoughness]->SRV();
			shader_views[2] = depth_target->SRV();
			shader_views[3] = lights->SRV();
			shader_views[4] = light_list->SRV();
			shader_views[5] = light_grid->SRV();
			command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, shader_views);

			command_context->SetInputLayout(nullptr);
			command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
			ShaderManager::GetShaderProgram(ShaderProgram::ClusterLightingPBR)->Bind(command_context);
			command_context->Draw(4);

			command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, ARRAYSIZE(shader_views));

			//Volumetric lighting for non-shadow casting lights
			std::vector<Light> volumetric_lights{};
			auto light_view = reg.view<Light>();
			for (auto e : light_view)
			{
				auto const& light = light_view.get(e);
				if (light.volumetric && !light.casts_shadows) volumetric_lights.push_back(light);
			}
			if (volumetric_lights.empty()) return;

			for (auto const& light : volumetric_lights)
			{
				ADRIA_ASSERT(light.volumetric);
				light_cbuf_data.casts_shadows = light.casts_shadows;
				light_cbuf_data.color = light.color * light.energy;
				light_cbuf_data.direction = light.direction;
				light_cbuf_data.inner_cosine = light.inner_cosine;
				light_cbuf_data.outer_cosine = light.outer_cosine;
				light_cbuf_data.position = light.position;
				light_cbuf_data.range = light.range;
				light_cbuf_data.volumetric_strength = light.volumetric_strength;

				Matrix camera_view = camera->View();
				light_cbuf_data.position = Vector4::Transform(light_cbuf_data.position, camera_view);
				light_cbuf_data.direction = Vector4::Transform(light_cbuf_data.direction, camera_view);
				light_cbuffer->Update(gfx->GetCommandContext(), light_cbuf_data);

				PassVolumetric(light);
			}
		}
		command_context->EndRenderPass();
		command_context->SetBlendState(nullptr);
	}
	void Renderer::PassForward()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Forward Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Forward Pass");

		command_context->BeginRenderPass(forward_pass); 
		PassForwardCommon(false);
		PassSky();
		PassOcean();
		PassAABB();
		PassForwardCommon(true);
		command_context->EndRenderPass();
	}

	void Renderer::PassVoxelize()
	{
		return;
		ADRIA_ASSERT_MSG(false, "Voxel GI is not working");
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Voxelization Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Voxelization Pass");

		std::vector<LightSBuffer> _lights{};
		auto light_view = reg.view<Light>();
		for (auto e : light_view)
		{
			LightSBuffer light_data{};
			auto& light = light_view.get(e);
			if (!light.active) continue;
			light_data.active = light.active;
			light_data.color = light.color * light.energy;
			light_data.position = light.position;
			light_data.direction = light.direction;
			light_data.range = light.range;
			light_data.type = static_cast<int>(light.type);
			light_data.inner_cosine = light.inner_cosine;
			light_data.outer_cosine = light.outer_cosine;
			light_data.casts_shadows = light.casts_shadows;
			_lights.push_back(light_data);

			if (light.type == LightType::Directional && light.casts_shadows && renderer_settings.voxel_debug)
				PassShadowMapDirectional(light);
		}
		lights->Update(_lights.data(), std::min<Uint64>(_lights.size(), VOXELIZE_MAX_LIGHTS) * sizeof(LightSBuffer));

		auto voxel_view = reg.view<Mesh, Transform, Material, Deferred, AABB>();

		ShaderManager::GetShaderProgram(ShaderProgram::Voxelize)->Bind(command_context);
		command_context->SetRasterizerState(cull_none.get());
		command_context->SetViewport(0, 0, VOXEL_RESOLUTION, VOXEL_RESOLUTION);

		GfxShaderResourceRW voxels_uav = voxels->UAV();
		command_context->GetNative()->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 1, &voxels_uav, nullptr);

		GfxShaderResourceRO lights_srv = lights->SRV();
		command_context->SetShaderResourceRO(GfxShaderStage::PS, 10, lights_srv);

		for (auto e : voxel_view)
		{
			auto [mesh, transform, material, aabb] = voxel_view.get<Mesh, Transform, Material, AABB>(e);

			if (!aabb.camera_visible) continue;

			object_cbuf_data.model = transform.current_transform;
			object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
			object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);

			material_cbuf_data.albedo_factor = material.albedo_factor;
			material_cbuffer->Update(gfx->GetCommandContext(), material_cbuf_data);
			auto view = g_TextureManager.GetTextureView(material.albedo_texture);
			command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_DIFFUSE, view);
			mesh.Draw(command_context);
		}

		command_context->GetNative()->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr,
																				0, 0, nullptr, nullptr);

		command_context->SetRasterizerState(nullptr);
		ShaderManager::GetShaderProgram(ShaderProgram::Voxelize)->Unbind(command_context);

		GfxShaderResourceRW uavs[] = { voxels_uav, voxel_texture->UAV() };
		command_context->SetShaderResourcesRW(0, uavs);
		ShaderManager::GetShaderProgram(ShaderProgram::VoxelCopy)->Bind(command_context);
		command_context->Dispatch(VOXEL_RESOLUTION * VOXEL_RESOLUTION * VOXEL_RESOLUTION / 256, 1, 1);
		ShaderManager::GetShaderProgram(ShaderProgram::VoxelCopy)->Unbind(command_context);

		command_context->UnsetShaderResourcesRW(0, ARRAYSIZE(uavs));
		command_context->GenerateMips(voxel_texture->SRV());

		if (renderer_settings.voxel_second_bounce)
		{
			command_context->SetShaderResourceRW(0, voxel_texture_second_bounce->UAV());
			GfxShaderResourceRO srvs[] = { voxels->SRV(), voxel_texture->SRV() };
			command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srvs);

			ShaderManager::GetShaderProgram(ShaderProgram::VoxelSecondBounce)->Bind(command_context);
			command_context->Dispatch(VOXEL_RESOLUTION / 8, VOXEL_RESOLUTION / 8, VOXEL_RESOLUTION / 8);
			ShaderManager::GetShaderProgram(ShaderProgram::VoxelSecondBounce)->Unbind(command_context);

			command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, ARRAYSIZE(srvs));
			command_context->SetShaderResourceRW(0, nullptr);
			command_context->GenerateMips(voxel_texture_second_bounce->SRV());
		}
	}
	void Renderer::PassVoxelizeDebug()
	{
		return;
		ADRIA_ASSERT_MSG(false, "Voxel GI is not working");
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Voxelization Debug Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Voxelization Debug Pass");

		command_context->BeginRenderPass(voxel_debug_pass);
		{
			GfxShaderResourceRO voxel_srv = voxel_texture->SRV();
			command_context->SetShaderResourceRO(GfxShaderStage::VS, 9, voxel_srv);

			ShaderManager::GetShaderProgram(ShaderProgram::VoxelizeDebug)->Bind(command_context);
			command_context->SetInputLayout(nullptr);
			command_context->SetTopology(GfxPrimitiveTopology::PointList);
			command_context->Draw(VOXEL_RESOLUTION * VOXEL_RESOLUTION * VOXEL_RESOLUTION);
			ShaderManager::GetShaderProgram(ShaderProgram::VoxelizeDebug)->Unbind(command_context);

			command_context->SetShaderResourceRO(GfxShaderStage::VS, 9, nullptr);
		}
		command_context->EndRenderPass();
	}
	void Renderer::PassVoxelGI()
	{
		return;
		ADRIA_ASSERT_MSG(false, "Voxel GI is not working");
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Voxel GI Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Voxel GI Pass");

		command_context->SetBlendState(additive_blend.get());
		command_context->BeginRenderPass(lighting_pass);
		{
			GfxShaderResourceRO shader_views[3] = { nullptr };
			shader_views[0] = gbuffer[GBufferSlot_NormalMetallic]->SRV();
			shader_views[1] = depth_target->SRV();
			shader_views[2] = renderer_settings.voxel_second_bounce ? voxel_texture_second_bounce->SRV() : voxel_texture->SRV();
			command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, shader_views);
			command_context->SetInputLayout(nullptr);
			command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
			ShaderManager::GetShaderProgram(ShaderProgram::VoxelGI)->Bind(command_context);
			command_context->Draw(4);
			ShaderManager::GetShaderProgram(ShaderProgram::VoxelGI)->Unbind(command_context);
			command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, ARRAYSIZE(shader_views));
		}
		command_context->EndRenderPass();
		command_context->SetBlendState(nullptr);
	}

	void Renderer::PassPostprocessing()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Postprocessing Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Postprocessing Pass");

		PassMotionVectors();

		auto lights = reg.view<Light>();
		command_context->BeginRenderPass(postprocess_passes[postprocess_index]);
		CopyTexture(hdr_render_target.get());
		command_context->SetBlendState(additive_blend.get());
		for (entity light : lights)
		{
			auto const& light_data = lights.get(light);
			if (!light_data.active || !light_data.lens_flare) continue;
			PassLensFlare(light_data);
		}
		command_context->SetBlendState(nullptr);

		command_context->EndRenderPass();
		postprocess_index = !postprocess_index;

		if (renderer_settings.clouds)
		{
			command_context->BeginRenderPass(postprocess_passes[postprocess_index]);
			PassVolumetricClouds();
			command_context->EndRenderPass();

			postprocess_index = !postprocess_index;
		}

		if (renderer_settings.fog)
		{
			command_context->BeginRenderPass(postprocess_passes[postprocess_index]);
			PassFog();
			command_context->EndRenderPass();

			postprocess_index = !postprocess_index;
		}

		if (renderer_settings.ssr)
		{
			command_context->BeginRenderPass(postprocess_passes[postprocess_index]);
			PassSSR();
			command_context->EndRenderPass();

			postprocess_index = !postprocess_index;
		}

		if (renderer_settings.vignette_enabled || renderer_settings.film_grain_enabled
			|| renderer_settings.chromatic_aberration_enabled || renderer_settings.lens_distortion_enabled)
		{
			command_context->BeginRenderPass(postprocess_passes[postprocess_index]);
			PassFilmEffects();
			command_context->EndRenderPass();

			postprocess_index = !postprocess_index;
		}

		if (renderer_settings.dof)
		{
			command_context->BeginRenderPass(postprocess_passes[postprocess_index]);
			PassDepthOfField();
			command_context->EndRenderPass();

			postprocess_index = !postprocess_index;
		}

		if (renderer_settings.bloom)
		{
			PassBloom();
			postprocess_index = !postprocess_index;
		}

		if (renderer_settings.motion_blur)
		{
			command_context->BeginRenderPass(postprocess_passes[postprocess_index]);
			PassMotionBlur();
			command_context->EndRenderPass();

			postprocess_index = !postprocess_index;
		}

		for (entity light : lights)
		{
			auto const& light_data = lights.get(light);
			if (!light_data.active) continue;

			if (light_data.type == LightType::Directional)
			{
				if (light_data.god_rays)
				{
					DrawSun(light);
					command_context->BeginRenderPass(postprocess_passes[!postprocess_index]);
					PassGodRays(light_data);
					command_context->EndRenderPass();
				}
				else
				{
					DrawSun(light);
					command_context->BeginRenderPass(postprocess_passes[!postprocess_index]);
					command_context->SetBlendState(additive_blend.get());
					CopyTexture(sun_target.get());
					command_context->SetBlendState(nullptr);
					command_context->EndRenderPass();
				}
			}
		}

		if (renderer_settings.anti_aliasing & AntiAliasing_TAA)
		{
			command_context->BeginRenderPass(postprocess_passes[postprocess_index]);
			PassTAA();
			command_context->EndRenderPass(); 
			postprocess_index = !postprocess_index;

			GfxRenderTarget rtvs[] = { prev_hdr_render_target->RTV() };
			command_context->SetRenderTargets(rtvs, nullptr);
			CopyTexture(postprocess_textures[!postprocess_index].get());
			command_context->SetRenderTargets({}, nullptr);
		}
	}

	void Renderer::PassShadowMapDirectional(Light const& light)
	{
		ADRIA_ASSERT(light.type == LightType::Directional);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Directional Shadow Map Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Directional Shadow Map Pass");
		
		auto const& [V, P] = scene_bounding_sphere ? LightViewProjection_Directional(light, *scene_bounding_sphere, light_bounding_box)
			: LightViewProjection_Directional(light, *camera, light_bounding_box);
		shadow_cbuf_data.lightview = V;
		shadow_cbuf_data.lightviewprojection = V * P;
		shadow_cbuf_data.shadow_map_size = SHADOW_MAP_SIZE;
		shadow_cbuf_data.softness = renderer_settings.shadow_softness;
		shadow_cbuf_data.shadow_matrices[0] = camera->View().Invert() * shadow_cbuf_data.lightviewprojection;
		shadow_cbuffer->Update(gfx->GetCommandContext(), shadow_cbuf_data);

		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, TEXTURE_SLOT_SHADOW, 1);
		command_context->BeginRenderPass(shadow_map_pass);
		{
			command_context->SetRasterizerState(shadow_depth_bias.get());
			LightFrustumCulling(LightType::Directional);
			PassShadowMapCommon();
			command_context->SetRasterizerState(nullptr);
		}
		command_context->EndRenderPass();
		GfxShaderResourceRO shadow_depth_srv[1] = { shadow_depth_map->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, TEXTURE_SLOT_SHADOW, shadow_depth_srv);
	}
	void Renderer::PassShadowMapSpot(Light const& light)
	{
		ADRIA_ASSERT(light.type == LightType::Spot);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Spot Shadow Map Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Spot Shadow Map Pass");

		auto const& [V, P] = LightViewProjection_Spot(light, light_bounding_frustum);
		shadow_cbuf_data.lightview = V;
		shadow_cbuf_data.lightviewprojection = V * P;
		shadow_cbuf_data.shadow_map_size = SHADOW_MAP_SIZE;
		shadow_cbuf_data.softness = renderer_settings.shadow_softness;
		shadow_cbuf_data.shadow_matrices[0] = camera->View().Invert() * shadow_cbuf_data.lightviewprojection;
		shadow_cbuffer->Update(gfx->GetCommandContext(), shadow_cbuf_data);

		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, TEXTURE_SLOT_SHADOW, 1);
		command_context->BeginRenderPass(shadow_map_pass);
		{
			command_context->SetRasterizerState(shadow_depth_bias.get());
			LightFrustumCulling(LightType::Spot);
			PassShadowMapCommon();
			command_context->SetRasterizerState(nullptr);
		}
		command_context->EndRenderPass();
		GfxShaderResourceRO shadow_depth_srv[1] = { shadow_depth_map->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, TEXTURE_SLOT_SHADOW, shadow_depth_srv);
	}
	void Renderer::PassShadowMapPoint(Light const& light)
	{
		ADRIA_ASSERT(light.type == LightType::Point);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Point Shadow Map Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Point Shadow Map Pass");

		for (Uint32 i = 0; i < shadow_cubemap_pass.size(); ++i)
		{
			auto const& [V, P] = LightViewProjection_Point(light, i, light_bounding_frustum);
			shadow_cbuf_data.lightviewprojection = V * P;
			shadow_cbuf_data.lightview = V;
			shadow_cbuffer->Update(gfx->GetCommandContext(), shadow_cbuf_data);

			command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, TEXTURE_SLOT_SHADOWCUBE, 1);
			command_context->BeginRenderPass(shadow_cubemap_pass[i]);
			{
				command_context->SetRasterizerState(shadow_depth_bias.get());
				LightFrustumCulling(LightType::Point);
				PassShadowMapCommon();
				command_context->SetRasterizerState(nullptr);
			}
			command_context->EndRenderPass();
		}

		GfxShaderResourceRO srv[] = { shadow_depth_cubemap->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, TEXTURE_SLOT_SHADOWCUBE, srv);
	}
	void Renderer::PassShadowMapCascades(Light const& light)
	{
		ADRIA_ASSERT(light.type == LightType::Directional);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Cascades Shadow Map Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Cascades Shadow Map Pass");

		std::array<Float, CASCADE_COUNT> split_distances;
		std::array<Matrix, CASCADE_COUNT> proj_matrices = RecalculateProjectionMatrices(*camera, renderer_settings.split_lambda, split_distances);
		std::array<Matrix, CASCADE_COUNT> light_view_projections{};

		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, TEXTURE_SLOT_SHADOWARRAY, 1);
		command_context->SetRasterizerState(shadow_depth_bias.get());
		
		for (Uint32 i = 0; i < CASCADE_COUNT; ++i)
		{
			auto const& [V, P] = LightViewProjection_Cascades(light, *camera, proj_matrices[i], light_bounding_box);
			light_view_projections[i] = V * P;
			shadow_cbuf_data.lightview = V;
			shadow_cbuf_data.lightviewprojection = light_view_projections[i];
			shadow_cbuffer->Update(gfx->GetCommandContext(), shadow_cbuf_data);

			command_context->BeginRenderPass(cascade_shadow_pass[i]);
			{
				LightFrustumCulling(LightType::Directional);
				PassShadowMapCommon();
			}
			command_context->EndRenderPass();
		}

		command_context->SetRasterizerState(nullptr);
		GfxShaderResourceRO srv[] = { shadow_cascade_maps->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, TEXTURE_SLOT_SHADOWARRAY, srv);

		shadow_cbuf_data.shadow_map_size = SHADOW_CASCADE_SIZE;
		shadow_cbuf_data.shadow_matrices[0] = camera->View().Invert() * light_view_projections[0];
		shadow_cbuf_data.shadow_matrices[1] = camera->View().Invert() * light_view_projections[1];
		shadow_cbuf_data.shadow_matrices[2] = camera->View().Invert() * light_view_projections[2];
		shadow_cbuf_data.shadow_matrices[3] = camera->View().Invert() * light_view_projections[3];
		shadow_cbuf_data.split0 = split_distances[0];
		shadow_cbuf_data.split1 = split_distances[1];
		shadow_cbuf_data.split2 = split_distances[2];
		shadow_cbuf_data.split3 = split_distances[3];
		shadow_cbuf_data.softness = renderer_settings.shadow_softness;
		shadow_cbuf_data.visualize = static_cast<Sint32>(false);
		shadow_cbuffer->Update(gfx->GetCommandContext(), shadow_cbuf_data);
	}
	void Renderer::PassShadowMapCommon()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		auto shadow_view = reg.view<Mesh, Transform, AABB>();
		if (!renderer_settings.shadow_transparent)
		{
			ShaderManager::GetShaderProgram(ShaderProgram::DepthMap)->Bind(command_context);
			for (auto e : shadow_view)
			{
				auto& aabb = shadow_view.get<AABB>(e);
				if (aabb.light_visible)
				{
					auto const& transform = shadow_view.get<Transform>(e);
					auto const& mesh = shadow_view.get<Mesh>(e);

					Matrix parent_transform = Matrix::Identity;
					if (Relationship* relationship = reg.get_if<Relationship>(e))
					{
						if (auto* root_transform = reg.get_if<Transform>(relationship->parent)) parent_transform = root_transform->current_transform;
					}

					object_cbuf_data.model = transform.current_transform * parent_transform;
					object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
					object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);
					mesh.Draw(command_context);
				}
			}
		}
		else
		{
			std::vector<entity> potentially_transparent, not_transparent;
			for (auto e : shadow_view)
			{
				auto const& aabb = shadow_view.get<AABB>(e);
				if (aabb.light_visible)
				{
					if (auto* p_material = reg.get_if<Material>(e))
					{
						if (p_material->albedo_texture != INVALID_TEXTURE_HANDLE)
							potentially_transparent.push_back(e);
						else not_transparent.push_back(e);
					}
					else not_transparent.push_back(e);
				}
			}

			ShaderManager::GetShaderProgram(ShaderProgram::DepthMap)->Bind(command_context);
			for (auto e : not_transparent)
			{
				auto& transform = shadow_view.get<Transform>(e);
				auto& mesh = shadow_view.get<Mesh>(e);

				Matrix parent_transform = Matrix::Identity;
				if (Relationship* relationship = reg.get_if<Relationship>(e))
				{
					if (auto* root_transform = reg.get_if<Transform>(relationship->parent)) parent_transform = root_transform->current_transform;
				}

				object_cbuf_data.model = transform.current_transform * parent_transform;
				object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
				object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);
				mesh.Draw(command_context);
			}

			ShaderManager::GetShaderProgram(ShaderProgram::DepthMap_Transparent)->Bind(command_context);
			for (auto e : potentially_transparent)
			{
				auto& transform = shadow_view.get<Transform>(e);
				auto& mesh = shadow_view.get<Mesh>(e);
				auto* material = reg.get_if<Material>(e);
				ADRIA_ASSERT(material != nullptr);
				ADRIA_ASSERT(material->albedo_texture != INVALID_TEXTURE_HANDLE);

				Matrix parent_transform = Matrix::Identity;
				if (Relationship* relationship = reg.get_if<Relationship>(e))
				{
					if (auto* root_transform = reg.get_if<Transform>(relationship->parent)) parent_transform = root_transform->current_transform;
				}

				object_cbuf_data.model = transform.current_transform * parent_transform;
				object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
				object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);

				auto view = g_TextureManager.GetTextureView(material->albedo_texture);
				command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_DIFFUSE, view);
				mesh.Draw(command_context);
			}
		}
	}

	void Renderer::PassVolumetric(Light const& light)
	{
		if (light.type == LightType::Directional && !light.casts_shadows)
		{
			ADRIA_LOG(WARNING, "Volumetric Directional Light that does not cast shadows does not make sense!");
			return;
		}
		ADRIA_ASSERT(light.volumetric);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Volumetric Lighting Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Volumetric Lighting Pass");

		GfxShaderResourceRO srv[] = { depth_target->SRV() };
		command_context->SetShaderResourceRO(GfxShaderStage::PS, 2, depth_target->SRV());

		command_context->SetInputLayout(nullptr);
		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
		switch (light.type)
		{
		case LightType::Directional:
			if (light.use_cascades) ShaderManager::GetShaderProgram(ShaderProgram::Volumetric_DirectionalCascades)->Bind(command_context);
			else ShaderManager::GetShaderProgram(ShaderProgram::Volumetric_Directional)->Bind(command_context);
			break;
		case LightType::Spot:
			ShaderManager::GetShaderProgram(ShaderProgram::Volumetric_Spot)->Bind(command_context);
			break;
		case LightType::Point:
			ShaderManager::GetShaderProgram(ShaderProgram::Volumetric_Point)->Bind(command_context);
			break;
		default:
			ADRIA_ASSERT(false && "Invalid Light Type!");
		}
		command_context->Draw(4);
		command_context->SetShaderResourceRO(GfxShaderStage::PS, 2, nullptr);
	}
	void Renderer::PassSky()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Sky Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Sky Pass");

		object_cbuf_data.model = Matrix::CreateTranslation(camera->Position());
		object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
		object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);

		command_context->SetRasterizerState(cull_none.get());
		command_context->SetDepthStencilState(leq_depth.get(), 0);

		auto skybox_view = reg.view<Skybox>();
		switch (renderer_settings.sky_type)
		{
		case SkyType::Skybox:
			ShaderManager::GetShaderProgram(ShaderProgram::Skybox)->Bind(command_context);
			for (auto e : skybox_view)
			{
				auto const& skybox = skybox_view.get(e);
				if (!skybox.active) continue;
				auto view = g_TextureManager.GetTextureView(skybox.cubemap_texture);
				command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_CUBEMAP, view);
				break;
			}
			break;
		case SkyType::UniformColor:
			ShaderManager::GetShaderProgram(ShaderProgram::UniformColorSky)->Bind(command_context);
			break;
		case SkyType::HosekWilkie:
			ShaderManager::GetShaderProgram(ShaderProgram::HosekWilkieSky)->Bind(command_context);
			break;
		default:
			ADRIA_ASSERT(false);
		}

		command_context->SetTopology(GfxPrimitiveTopology::TriangleList);
		command_context->SetVertexBuffer(cube_vb.get());
		command_context->SetIndexBuffer(cube_ib.get());
		command_context->DrawIndexed(cube_ib->GetCount());
		command_context->SetDepthStencilState(nullptr, 0);
		command_context->SetRasterizerState(nullptr);
	}
	void Renderer::PassOcean()
	{
		if (reg.size<Ocean>() == 0) return;

		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Ocean Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Ocean Pass");

		if (renderer_settings.ocean_wireframe) command_context->SetRasterizerState(wireframe.get());
		else command_context->SetRasterizerState(cull_none.get());

		command_context->SetBlendState(alpha_blend.get());

		auto skyboxes = reg.view<Skybox>();
		GfxShaderResourceRO skybox_srv = nullptr;
		for (auto skybox : skyboxes)
		{
			auto const& _skybox = skyboxes.get(skybox);
			skybox_srv = g_TextureManager.GetTextureView(_skybox.cubemap_texture);
			if (skybox_srv) break;
		}

		GfxShaderResourceRO displacement_map_srv = ping_pong_spectrum_textures[!pong_spectrum]->SRV();
		renderer_settings.ocean_tesselation
			? command_context->SetShaderResourceRO(GfxShaderStage::DS, 0, displacement_map_srv) :
			  command_context->SetShaderResourceRO(GfxShaderStage::VS, 0, displacement_map_srv);

		GfxShaderResourceRO srvs[] = { ocean_normal_map->SRV(), skybox_srv , g_TextureManager.GetTextureView(foam_handle) };
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srvs);

		renderer_settings.ocean_tesselation ? ShaderManager::GetShaderProgram(ShaderProgram::OceanLOD)->Bind(command_context)
											: ShaderManager::GetShaderProgram(ShaderProgram::Ocean)->Bind(command_context);

		auto ocean_chunk_view = reg.view<Mesh, Material, Transform, AABB, Ocean>();
		for (auto ocean_chunk : ocean_chunk_view)
		{
			auto [mesh, material, transform, aabb] = ocean_chunk_view.get<const Mesh, const Material, const Transform, const AABB>(ocean_chunk);

			if (aabb.camera_visible)
			{
				object_cbuf_data.model = transform.current_transform;
				object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
				object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);
				
				material_cbuf_data.diffuse = material.diffuse;
				material_cbuffer->Update(gfx->GetCommandContext(), material_cbuf_data);

				renderer_settings.ocean_tesselation ? mesh.Draw(command_context, GfxPrimitiveTopology::PatchList3) : mesh.Draw(command_context);
			}
		}

		renderer_settings.ocean_tesselation ? ShaderManager::GetShaderProgram(ShaderProgram::OceanLOD)->Unbind(command_context) : ShaderManager::GetShaderProgram(ShaderProgram::Ocean)->Unbind(command_context);

		static GfxShaderResourceRO null_srv = nullptr;
		renderer_settings.ocean_tesselation ? 
			command_context->SetShaderResourceRO(GfxShaderStage::DS, 0, nullptr) :
			command_context->SetShaderResourceRO(GfxShaderStage::VS, 0, nullptr);
		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, 3);

		command_context->SetBlendState(nullptr);
		command_context->SetRasterizerState(nullptr);
	}
	void Renderer::PassParticles()
	{
		if (reg.size<Emitter>() == 0) return;
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Particles Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Particles Pass");

		command_context->BeginRenderPass(particle_pass);
		command_context->SetBlendState(alpha_blend.get());

		auto emitters = reg.view<Emitter>();
		for (auto emitter : emitters)
		{
			Emitter const& emitter_params = emitters.get(emitter);
			particle_renderer.Render(emitter_params, depth_target->SRV(), g_TextureManager.GetTextureView(emitter_params.particle_texture));
		}

		command_context->SetBlendState(nullptr);
		command_context->EndRenderPass();
	}
	void Renderer::PassAABB()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		auto aabb_view = reg.view<AABB>();
		for (auto e : aabb_view)
		{
			auto& aabb = aabb_view.get(e);
			if (aabb.draw_aabb)
			{
				object_cbuf_data.model = Matrix::Identity;
				object_cbuf_data.transposed_inverse_model = Matrix::Identity;
				object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);

				material_cbuf_data.diffuse = Vector3(1, 0, 0);
				material_cbuffer->Update(gfx->GetCommandContext(), material_cbuf_data);

				command_context->SetRasterizerState(wireframe.get());
				command_context->SetDepthStencilState(no_depth_test.get(), 0);
				ShaderManager::GetShaderProgram(ShaderProgram::Solid)->Bind(command_context);
				command_context->SetTopology(GfxPrimitiveTopology::LineList);
				command_context->SetVertexBuffer(aabb.aabb_vb.get());
				command_context->SetIndexBuffer(aabb_wireframe_ib.get());
				command_context->DrawIndexed(aabb_wireframe_ib->GetCount());
				command_context->SetRasterizerState(nullptr);
				command_context->SetDepthStencilState(nullptr, 0);

				aabb.draw_aabb = false;
				break;
			}
		}
	}

	void Renderer::PassForwardCommon(Bool transparent)
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		auto forward_view = reg.view<Mesh, Transform, AABB, Material, Forward>();

		if (transparent) command_context->SetBlendState(alpha_blend.get());
		for (auto e : forward_view)
		{
			auto [forward, aabb] = forward_view.get<Forward const, AABB const>(e);

			if (!(aabb.camera_visible && forward.transparent == transparent)) continue;
			
			auto [transform, mesh, material] = forward_view.get<Transform, Mesh, Material>(e);

			ShaderManager::GetShaderProgram(material.shader)->Bind(command_context);

			object_cbuf_data.model = transform.current_transform;
			object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
			object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);
				
			material_cbuf_data.diffuse = material.diffuse;
			material_cbuf_data.albedo_factor = material.albedo_factor;
			material_cbuffer->Update(gfx->GetCommandContext(), material_cbuf_data);

			if (material.albedo_texture != INVALID_TEXTURE_HANDLE)
			{
				auto view = g_TextureManager.GetTextureView(material.albedo_texture);
				command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_DIFFUSE, view);
			}

			auto const* states = reg.get_if<RenderState>(e);
			if (states) ResolveCustomRenderState(*states, false);
			mesh.Draw(command_context);
			if (states) ResolveCustomRenderState(*states, true);
		}

		if (transparent) command_context->SetBlendState(nullptr);
	}

	void Renderer::PassLensFlare(Light const& light)
	{
		ADRIA_ASSERT(light.lens_flare);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Lens Flare Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Lens Flare Pass");

		if (light.type != LightType::Directional) 
		{
			ADRIA_LOG(WARNING, "Using Lens Flare on a Non-Directional Light Source");
		}

		Vector4 light_ss{};
		{
			Vector4 light_pos = Vector4::Transform(light.position, camera->ViewProj());
			light_ss.x = 0.5f * light_pos.x / light_pos.w + 0.5f;
			light_ss.y = -0.5f * light_pos.y / light_pos.w + 0.5f;
			light_ss.z = light_pos.z / light_pos.w;
			light_ss.w = 1.0f;
		}
		light_cbuf_data.screenspace_position = light_ss;
		light_cbuffer->Update(gfx->GetCommandContext(), light_cbuf_data);

		{
			GfxShaderResourceRO depth_srv_array[1] = { depth_target->SRV() };
			command_context->SetShaderResourceRO(GfxShaderStage::GS, 7, depth_target->SRV());
			command_context->SetShaderResourcesRO(GfxShaderStage::GS, 0, lens_flare_textures);
			command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, lens_flare_textures);

			ShaderManager::GetShaderProgram(ShaderProgram::LensFlare)->Bind(command_context);
			command_context->SetInputLayout(nullptr);
			command_context->SetTopology(GfxPrimitiveTopology::PointList);
			command_context->Draw(7);
			ShaderManager::GetShaderProgram(ShaderProgram::LensFlare)->Unbind(command_context);

			command_context->UnsetShaderResourcesRO(GfxShaderStage::GS, 0, 8);
			command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, 7);
		}
	}
	void Renderer::PassVolumetricClouds()
	{
		ADRIA_ASSERT(renderer_settings.clouds);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Volumetric Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Volumetric Pass");

		GfxShaderResourceRO srv_array[] = { clouds_textures[0], clouds_textures[1], clouds_textures[2], depth_target->SRV()};
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srv_array);
		command_context->SetInputLayout(nullptr);

		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
		ShaderManager::GetShaderProgram(ShaderProgram::Volumetric_Clouds)->Bind(command_context);
		command_context->Draw(4);

		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, 4);

		command_context->EndRenderPass();
		postprocess_index = !postprocess_index;
		command_context->BeginRenderPass(postprocess_passes[postprocess_index]);

		BlurTexture(postprocess_textures[!postprocess_index].get());
		command_context->SetBlendState(alpha_blend.get());
		CopyTexture(blur_texture_final.get());
		command_context->SetBlendState(nullptr);
	}
	void Renderer::PassSSR()
	{
		ADRIA_ASSERT(renderer_settings.ssr);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "SSR Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "SSR Pass");

		postprocess_cbuf_data.ssr_ray_hit_threshold = renderer_settings.ssr_ray_hit_threshold;
		postprocess_cbuf_data.ssr_ray_step = renderer_settings.ssr_ray_step;
		postprocess_cbuffer->Update(gfx->GetCommandContext(), postprocess_cbuf_data);

		GfxShaderResourceRO srv_array[] = { gbuffer[GBufferSlot_NormalMetallic]->SRV(), postprocess_textures[!postprocess_index]->SRV(), depth_target->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srv_array);
		command_context->SetInputLayout(nullptr);

		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
		ShaderManager::GetShaderProgram(ShaderProgram::SSR)->Bind(command_context);
		command_context->Draw(4);
		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, 3);
	}
	void Renderer::PassGodRays(Light const& light)
	{
		ADRIA_ASSERT(light.god_rays);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "God Rays Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "God Rays Pass");

		if (light.type != LightType::Directional)
		{
			ADRIA_LOG(WARNING, "Using God Rays on a Non-Directional Light Source");
			return;
		}
		{
			light_cbuf_data.godrays_decay = light.godrays_decay;
			light_cbuf_data.godrays_weight = light.godrays_weight;
			light_cbuf_data.godrays_density = light.godrays_density;
			light_cbuf_data.godrays_exposure = light.godrays_exposure;
			light_cbuf_data.color = light.color * light.energy;

			Vector4 camera_position(camera->Position());
			Vector4 light_position = light.type == LightType::Directional ? Vector4::Transform(light.position, XMMatrixTranslation(camera_position.x, 0.0f, camera_position.y)) : light.position;

			Vector4 light_ss{};
			{
				Vector4 light_pos = Vector4::Transform(light.position, camera->ViewProj());
				light_ss.x = 0.5f * light_pos.x / light_pos.w + 0.5f;
				light_ss.y = -0.5f * light_pos.y / light_pos.w + 0.5f;
				light_ss.z = light_pos.z / light_pos.w;
				light_ss.w = 1.0f;
			}
			light_cbuf_data.screenspace_position = light_ss;

			static Float const max_light_dist = 1.3f;

			Float f_max_dist = std::max(abs(light_cbuf_data.screenspace_position.x), abs(light_cbuf_data.screenspace_position.y));
			if (f_max_dist >= 1.0f) light_cbuf_data.color = Vector4::Transform(light_cbuf_data.color, Matrix::CreateScale((max_light_dist - f_max_dist), (max_light_dist - f_max_dist), (max_light_dist - f_max_dist)));

			light_cbuffer->Update(gfx->GetCommandContext(), light_cbuf_data);
		}

		command_context->SetBlendState(additive_blend.get());
		{
			GfxShaderResourceRO srv_array[1] = { sun_target->SRV() };
	
			command_context->SetShaderResourceRO(GfxShaderStage::PS, 0, sun_target->SRV());
			command_context->SetInputLayout(nullptr);
			command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
			ShaderManager::GetShaderProgram(ShaderProgram::GodRays)->Bind(command_context);
			command_context->Draw(4);
			command_context->SetShaderResourceRO(GfxShaderStage::PS, 0, nullptr);
		}
		command_context->SetBlendState(nullptr);
	}
	void Renderer::PassDepthOfField()
	{
		ADRIA_ASSERT(renderer_settings.dof);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Depth of Field Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Depth of Field Pass");

		if (renderer_settings.bokeh)
		{
			ShaderManager::GetShaderProgram(ShaderProgram::BokehGenerate)->Bind(command_context);
			GfxShaderResourceRO srv_array[2] = { postprocess_textures[!postprocess_index]->SRV(), depth_target->SRV() };
			Uint32 initial_count[] = { 0 };
			GfxShaderResourceRW bokeh_uav[] = { bokeh_buffer->UAV() };
			command_context->SetShaderResourcesRW(0, bokeh_uav, initial_count);
			command_context->SetShaderResourcesRO(GfxShaderStage::CS, 0, srv_array);
			command_context->Dispatch((Uint32)std::ceil(width / 32.0f), (Uint32)std::ceil(height / 32.0f), 1);

			command_context->UnsetShaderResourcesRW(0, 1);
			command_context->UnsetShaderResourcesRO(GfxShaderStage::CS, 0, 2);
			ShaderManager::GetShaderProgram(ShaderProgram::BokehGenerate)->Unbind(command_context);
		}

		BlurTexture(hdr_render_target.get());

		GfxShaderResourceRO srvs[3] = { postprocess_textures[!postprocess_index]->SRV(), blur_texture_final->SRV(), depth_target->SRV()};
		postprocess_cbuf_data.dof_params = XMVectorSet(renderer_settings.dof_near_blur, renderer_settings.dof_near, renderer_settings.dof_far, renderer_settings.dof_far_blur);
		postprocess_cbuffer->Update(gfx->GetCommandContext(), postprocess_cbuf_data);

		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srvs);
		command_context->SetInputLayout(nullptr);
		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
		ShaderManager::GetShaderProgram(ShaderProgram::DOF)->Bind(command_context);
		command_context->Draw(4);
		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, ARRAYSIZE(srvs));

		if (renderer_settings.bokeh)
		{
			command_context->CopyStructureCount(bokeh_indirect_draw_buffer.get(), 0, bokeh_buffer->UAV());
			GfxShaderResourceRO bokeh = nullptr;
			switch (renderer_settings.bokeh_type)
			{
			case BokehType::Hex:
				bokeh = g_TextureManager.GetTextureView(hex_bokeh_handle);
				break;
			case BokehType::Oct:
				bokeh = g_TextureManager.GetTextureView(oct_bokeh_handle);
				break;
			case BokehType::Circle:
				bokeh = g_TextureManager.GetTextureView(circle_bokeh_handle);
				break;
			case BokehType::Cross:
				bokeh = g_TextureManager.GetTextureView(cross_bokeh_handle);
				break;
			default:
				ADRIA_ASSERT(false && "Invalid Bokeh Type");
			}

			GfxShaderResourceRO bokeh_srv = bokeh_buffer->SRV();

			command_context->SetShaderResourceRO(GfxShaderStage::VS, 0, bokeh_srv);
			command_context->SetShaderResourceRO(GfxShaderStage::PS, 0, bokeh);
			command_context->SetVertexBuffer(nullptr);
			command_context->SetInputLayout(nullptr);
			command_context->SetTopology(GfxPrimitiveTopology::PointList);
			command_context->SetIndexBuffer(nullptr);

			ShaderManager::GetShaderProgram(ShaderProgram::BokehDraw)->Bind(command_context);
			command_context->SetBlendState(additive_blend.get());
			command_context->DrawIndirect(*bokeh_indirect_draw_buffer, 0);
			command_context->SetBlendState(nullptr);
			static GfxShaderResourceRO null_srv = nullptr;
			command_context->SetShaderResourceRO(GfxShaderStage::VS, 0, nullptr);
			command_context->SetShaderResourceRO(GfxShaderStage::PS, 0, nullptr);
			ShaderManager::GetShaderProgram(ShaderProgram::BokehDraw)->Unbind(command_context);
		}
	}
	void Renderer::PassBloom()
	{
		ADRIA_ASSERT(renderer_settings.bloom);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Bloom Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Bloom Pass");

		GfxShaderResourceRW uav[] = { bloom_extract_texture->UAV() };
		GfxShaderResourceRO srv[] = { postprocess_textures[!postprocess_index]->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::CS, 0, srv);
		command_context->SetShaderResourcesRW(0, uav);

		ShaderManager::GetShaderProgram(ShaderProgram::BloomExtract)->Bind(command_context);
		command_context->Dispatch((Uint32)std::ceil(width / 32.0f), (Uint32)std::ceil(height / 32.0f), 1);

		command_context->UnsetShaderResourcesRO(GfxShaderStage::CS, 0, ARRAYSIZE(srv));
		command_context->UnsetShaderResourcesRW(0, ARRAYSIZE(uav));
		
		command_context->GenerateMips(bloom_extract_texture->SRV());

		GfxShaderResourceRW uav2[] = { postprocess_textures[postprocess_index]->UAV() };
		GfxShaderResourceRO srv2[] = { postprocess_textures[!postprocess_index]->SRV(), bloom_extract_texture->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::CS, 0, srv2);
		command_context->SetShaderResourcesRW(0, uav2);

		ShaderManager::GetShaderProgram(ShaderProgram::BloomCombine)->Bind(command_context);
		command_context->Dispatch((Uint32)std::ceil(width / 32.0f), (Uint32)std::ceil(height / 32.0f), 1);

		command_context->UnsetShaderResourcesRO(GfxShaderStage::CS, 0, ARRAYSIZE(srv2));
		command_context->UnsetShaderResourcesRW(0, ARRAYSIZE(uav2));
	}
	void Renderer::PassMotionVectors()
	{
		if (!renderer_settings.motion_blur && !(renderer_settings.anti_aliasing & AntiAliasing_TAA)) return;
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Velocity Buffer Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Velocity Buffer Pass");

		postprocess_cbuf_data.velocity_buffer_scale = renderer_settings.velocity_buffer_scale;
		postprocess_cbuffer->Update(gfx->GetCommandContext(), postprocess_cbuf_data);

		command_context->BeginRenderPass(velocity_buffer_pass);
		{
			command_context->SetShaderResourceRO(GfxShaderStage::PS, 0, depth_target->SRV());
			command_context->SetInputLayout(nullptr);
			command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
			ShaderManager::GetShaderProgram(ShaderProgram::MotionVectors)->Bind(command_context);
			command_context->Draw(4);
			command_context->SetShaderResourceRO(GfxShaderStage::PS, 0,nullptr);
		}
		command_context->EndRenderPass();
	}

	void Renderer::PassMotionBlur()
	{
		ADRIA_ASSERT(renderer_settings.motion_blur);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Motion Blur Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Motion Blur Pass");

		GfxShaderResourceRO srvs[] = { postprocess_textures[!postprocess_index]->SRV(), velocity_buffer->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srvs);

		command_context->SetInputLayout(nullptr);
		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
		ShaderManager::GetShaderProgram(ShaderProgram::MotionBlur)->Bind(command_context);
		command_context->Draw(4);

		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, ARRAYSIZE(srvs));
	}
	void Renderer::PassFog()
	{
		ADRIA_ASSERT(renderer_settings.fog);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Fog Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Fog Pass");

		postprocess_cbuf_data.fog_falloff = renderer_settings.fog_falloff;
		postprocess_cbuf_data.fog_density = renderer_settings.fog_density;
		postprocess_cbuf_data.fog_type = static_cast<Sint32>(renderer_settings.fog_type);
		postprocess_cbuf_data.fog_start = renderer_settings.fog_start;
		postprocess_cbuf_data.fog_color = Vector4(renderer_settings.fog_color[0], renderer_settings.fog_color[1], renderer_settings.fog_color[2], 1);
		postprocess_cbuffer->Update(gfx->GetCommandContext(), postprocess_cbuf_data);

		GfxShaderResourceRO srvs[] = { postprocess_textures[!postprocess_index]->SRV(), depth_target->SRV() };

		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srvs);
		command_context->SetInputLayout(nullptr);
		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
		ShaderManager::GetShaderProgram(ShaderProgram::Fog)->Bind(command_context);
		command_context->Draw(4);

		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, ARRAYSIZE(srvs));
	}
	void Renderer::PassFilmEffects()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Film Effects Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Film Effects Pass");

		auto GetFilmGrainSeed = [](Float dt, Float seed_update_rate)
		{
			static Uint32 seed_counter = 0;
			static Float time_counter = 0.0;
			time_counter += dt;
			if (time_counter >= seed_update_rate)
			{
				++seed_counter;
				time_counter = 0.0;
			}
			return seed_counter;
		};

		postprocess_cbuf_data.lens_distortion_enabled = renderer_settings.lens_distortion_enabled;
		postprocess_cbuf_data.lens_distortion_intensity = renderer_settings.lens_distortion_intensity;
		postprocess_cbuf_data.chromatic_aberration_enabled = renderer_settings.chromatic_aberration_enabled;
		postprocess_cbuf_data.chromatic_aberration_intensity = renderer_settings.chromatic_aberration_intensity;
		postprocess_cbuf_data.vignette_enabled = renderer_settings.vignette_enabled;
		postprocess_cbuf_data.vignette_intensity = renderer_settings.vignette_intensity;
		postprocess_cbuf_data.film_grain_enabled = renderer_settings.film_grain_enabled;
		postprocess_cbuf_data.film_grain_scale = renderer_settings.film_grain_scale;
		postprocess_cbuf_data.film_grain_amount = renderer_settings.film_grain_amount;
		postprocess_cbuf_data.film_grain_seed = GetFilmGrainSeed(current_dt, renderer_settings.film_grain_seed_update_rate);
		postprocess_cbuffer->Update(gfx->GetCommandContext(), postprocess_cbuf_data);

		GfxShaderResourceRO srv_array[] = { postprocess_textures[!postprocess_index]->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srv_array);
		command_context->SetInputLayout(nullptr);
		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
		ShaderManager::GetShaderProgram(ShaderProgram::FilmEffects)->Bind(command_context);
		command_context->Draw(4);
		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, 1);
	}
	void Renderer::PassToneMap()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Tone Map Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Tone Map Pass");
		
		postprocess_cbuf_data.tone_map_exposure = renderer_settings.tone_map_exposure;
		postprocess_cbuffer->Update(gfx->GetCommandContext(), postprocess_cbuf_data);

		GfxShaderResourceRO srvs[] = { postprocess_textures[!postprocess_index]->SRV() };
		
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srvs);
		command_context->SetInputLayout(nullptr);
		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);

		switch (renderer_settings.tone_map_op)
		{
		case ToneMap::Reinhard:
			ShaderManager::GetShaderProgram(ShaderProgram::ToneMap_Reinhard)->Bind(command_context);
			break;
		case ToneMap::Linear:
			ShaderManager::GetShaderProgram(ShaderProgram::ToneMap_Linear)->Bind(command_context);
			break;
		case ToneMap::Hable:
			ShaderManager::GetShaderProgram(ShaderProgram::ToneMap_Hable)->Bind(command_context);
			break;
		case ToneMap::TonyMcMapface:
			command_context->SetShaderResourceRO(GfxShaderStage::PS, 1, g_TextureManager.GetTextureView(lut_tony_mcmapface_handle));
			ShaderManager::GetShaderProgram(ShaderProgram::ToneMap_TonyMcMapface)->Bind(command_context);
			break;
		default:
			ADRIA_ASSERT(false && "Unsupported Basic effect!");
		}

		command_context->Draw(4);
		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, ARRAYSIZE(srvs));
	}
	void Renderer::PassFXAA()
	{
		ADRIA_ASSERT(renderer_settings.anti_aliasing & AntiAliasing_FXAA);
		GfxCommandContext* command_context = gfx->GetCommandContext();
		ID3D11DeviceContext* context = command_context->GetNative();
		AdriaGfxProfileCondScope(command_context, "FXAA Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "FXAA Pass");

		GfxShaderResourceRO srvs[1] = { fxaa_texture->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srvs);

		command_context->SetInputLayout(nullptr);
		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
		ShaderManager::GetShaderProgram(ShaderProgram::FXAA)->Bind(command_context);
		command_context->Draw(4);

		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, ARRAYSIZE(srvs));
	}
	void Renderer::PassTAA()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "TAA Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "TAA Pass");

		GfxShaderResourceRO srvs[] = { postprocess_textures[!postprocess_index]->SRV(), prev_hdr_render_target->SRV(), velocity_buffer->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srvs);

		command_context->SetInputLayout(nullptr);
		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
		ShaderManager::GetShaderProgram(ShaderProgram::TAA)->Bind(command_context);
		command_context->Draw(4);
		
		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, ARRAYSIZE(srvs));
	}

	void Renderer::DrawSun(entity sun)
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxProfileCondScope(command_context, "Sun Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(command_context, "Sun Pass");

		GfxRenderTarget rtv = sun_target->RTV();
		GfxDepthTarget  dsv = depth_target->DSV();
		
		Float black[4] = { 0.0f };
		command_context->ClearRenderTarget(rtv, black);
		command_context->SetRenderTarget(rtv, dsv);
		command_context->SetBlendState(alpha_blend.get());
		{
			auto [transform, mesh, material] = reg.get<Transform, Mesh, Material>(sun);
			ShaderManager::GetShaderProgram(ShaderProgram::Sun)->Bind(command_context);

			object_cbuf_data.model = transform.current_transform;
			object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
			object_cbuffer->Update(gfx->GetCommandContext(), object_cbuf_data);
			material_cbuf_data.diffuse = material.diffuse;
			material_cbuf_data.albedo_factor = material.albedo_factor;
			material_cbuffer->Update(gfx->GetCommandContext(), material_cbuf_data);

			if (material.albedo_texture != INVALID_TEXTURE_HANDLE)
			{
				auto view = g_TextureManager.GetTextureView(material.albedo_texture);
				command_context->SetShaderResourceRO(GfxShaderStage::PS, TEXTURE_SLOT_DIFFUSE, view);
			}
			mesh.Draw(command_context);
		}
		command_context->SetBlendState(nullptr);
		command_context->SetRenderTarget(nullptr, nullptr);
	}

	void Renderer::BlurTexture(GfxTexture const* src)
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		std::array<GfxShaderResourceRW, 2>  uavs{};
		std::array<GfxShaderResourceRO, 2>  srvs{};
		
		uavs[0] = blur_texture_intermediate->UAV();  
		uavs[1] = blur_texture_final->UAV();  
		srvs[0] = blur_texture_intermediate->SRV();  
		srvs[1] = blur_texture_final->SRV(); 

		GfxShaderResourceRW blur_uav[1] = { nullptr };
		GfxShaderResourceRO blur_srv[1] = { nullptr };

		Uint32 width = src->GetDesc().width;
		Uint32 height = src->GetDesc().height;
		
		GfxShaderResourceRO src_srv = src->SRV();

		command_context->SetShaderResourceRO(GfxShaderStage::CS, 0, src_srv);
		command_context->SetShaderResourceRW(0, uavs[0]);
		ShaderManager::GetShaderProgram(ShaderProgram::Blur_Horizontal)->Bind(command_context);
		command_context->Dispatch((Uint32)std::ceil(width * 1.0f / 1024), height, 1);

		command_context->SetShaderResourceRO(GfxShaderStage::CS, 0, nullptr);
		command_context->SetShaderResourceRW(0, nullptr);

		command_context->SetShaderResourceRO(GfxShaderStage::CS, 0, srvs[0]);
		command_context->SetShaderResourceRW(0, uavs[1]);

		ShaderManager::GetShaderProgram(ShaderProgram::Blur_Vertical)->Bind(command_context);
		command_context->Dispatch(width, (Uint32)std::ceil(height * 1.0f / 1024), 1);
		
		command_context->SetShaderResourceRO(GfxShaderStage::CS, 0, nullptr);
		command_context->SetShaderResourceRW(0, nullptr);
	}
	void Renderer::CopyTexture(GfxTexture const* src)
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		
		command_context->SetShaderResourceRO(GfxShaderStage::PS, 0, src->SRV());
		command_context->SetInputLayout(nullptr);
		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
		ShaderManager::GetShaderProgram(ShaderProgram::Copy)->Bind(command_context);
		command_context->Draw(4);
		command_context->SetShaderResourceRO(GfxShaderStage::CS, 0, nullptr);
	}
	void Renderer::AddTextures(GfxTexture const* src1, GfxTexture const* src2)
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		
		GfxShaderResourceRO srv[] = { src1->SRV(), src2->SRV() };
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, srv);

		command_context->SetInputLayout(nullptr);
		command_context->SetTopology(GfxPrimitiveTopology::TriangleStrip);
		ShaderManager::GetShaderProgram(ShaderProgram::Add)->Bind(command_context);
		command_context->Draw(4);
		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, ARRAYSIZE(srv));		
	}

	void Renderer::ResolveCustomRenderState(RenderState const& state, Bool reset)
	{
		GfxCommandContext* context = gfx->GetCommandContext();

		if (reset)
		{
			if(state.blend_state != BlendState::None) context->SetBlendState(nullptr);
			if(state.depth_state != DepthState::None) context->SetDepthStencilState(nullptr, 0);
			if(state.raster_state != RasterizerState::None) context->SetRasterizerState(nullptr);
			return;
		}

		switch (state.blend_state)
		{
			case BlendState::AlphaBlend:
			{
				context->SetBlendState(alpha_blend.get());
				break;
			}
			case BlendState::AdditiveBlend:
			{
				context->SetBlendState(additive_blend.get());
				break;
			}
		}
	}

}

