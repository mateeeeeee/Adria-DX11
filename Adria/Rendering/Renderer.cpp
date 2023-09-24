#include <map>
#include "Renderer.h"
#include "Camera.h"
#include "Components.h"
#include "ShaderManager.h"
#include "SkyModel.h"
#include "Logging/Logger.h"
#include "Graphics/GfxDevice.h"
#include "Graphics/GfxCommonStates.h"
#include "Graphics/GfxScopedAnnotation.h"
#include "Math/Constants.h"
#include "Utilities/Random.h"
#include "DDSTextureLoader.h"

using namespace DirectX;

namespace adria
{
	using namespace tecs;

	namespace
	{
		constexpr uint32 SHADOW_MAP_SIZE = 2048;
		constexpr uint32 SHADOW_CUBE_SIZE = 512;
		constexpr uint32 SHADOW_CASCADE_SIZE = 2048;
		constexpr uint32 CASCADE_COUNT = 4;

		std::pair<Matrix, Matrix> LightViewProjection_Directional(Light const& light, Camera const& camera, BoundingBox& cull_box)
		{
			BoundingFrustum frustum = camera.Frustum();
			std::array<Vector3, BoundingFrustum::CORNER_COUNT> corners = {};
			frustum.GetCorners(corners.data());

			BoundingSphere frustum_sphere;
			BoundingSphere::CreateFromFrustum(frustum_sphere, frustum);

			Vector3 frustum_center(0, 0, 0);
			for (uint32 i = 0; i < corners.size(); ++i)
			{
				frustum_center = frustum_center + corners[i];
			}
			frustum_center /= static_cast<float>(corners.size());

			float radius = 0.0f;
			for (Vector3 const& corner : corners)
			{
				float distance = Vector3::Distance(corner, frustum_center);
				radius = std::max(radius, distance);
			}
			radius = std::ceil(radius * 8.0f) / 8.0f;

			Vector3 const max_extents(radius, radius, radius);
			Vector3 const min_extents = -max_extents;
			Vector3 const cascade_extents = max_extents - min_extents;

			Vector4 light_dir = light.direction; light_dir.Normalize();
			Matrix V = XMMatrixLookAtLH(frustum_center, frustum_center + 1.0f * light_dir * radius, Vector3::Up);

			float l = min_extents.x;
			float b = min_extents.y;
			float n = min_extents.z;
			float r = max_extents.x;
			float t = max_extents.y;
			float f = max_extents.z * 1.5f;

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
			static const float shadow_near = 0.5f;
			float fov_angle = 2.0f * acos(light.outer_cosine);
			Matrix P = XMMatrixPerspectiveFovLH(fov_angle, 1.0f, shadow_near, light.range);

			cull_frustum = BoundingFrustum(P);
			cull_frustum.Transform(cull_frustum, V.Invert());

			return { V,P };
		}
		std::pair<Matrix, Matrix> LightViewProjection_Point(Light const& light, uint32 face_index, BoundingFrustum& cull_frustum)
		{
			static float const shadow_near = 0.5f;
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
			static float const far_factor = 1.5f;
			static float const light_distance_factor = 1.0f;

			BoundingFrustum frustum(projection_matrix);
			frustum.Transform(frustum, camera.View().Invert());
			std::array<Vector3, BoundingFrustum::CORNER_COUNT> corners{};
			frustum.GetCorners(corners.data());

			Vector3 frustum_center(0, 0, 0);
			for (Vector3 const& corner : corners)
			{
				frustum_center = frustum_center + corner;
			}
			frustum_center /= static_cast<float>(corners.size());

			float radius = 0.0f;
			for (Vector3 const& corner : corners)
			{
				float distance = Vector3::Distance(corner, frustum_center);
				radius = std::max(radius, distance);
			}
			radius = std::ceil(radius * 8.0f) / 8.0f;

			Vector3 const max_extents(radius, radius, radius);
			Vector3 const min_extents = -max_extents;
			Vector3 const cascade_extents = max_extents - min_extents;

			Vector3 light_dir = XMVector3Normalize(light.direction);
			Matrix V = XMMatrixLookAtLH(frustum_center, frustum_center + light_distance_factor * light_dir * radius, Vector3::Up);

			float l = min_extents.x;
			float b = min_extents.y;
			float n = min_extents.z - far_factor * radius;
			float r = max_extents.x;
			float t = max_extents.y;
			float f = max_extents.z * far_factor;

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
			
			float l = sphere_centerLS.x - scene_bounding_sphere.Radius;
			float b = sphere_centerLS.y - scene_bounding_sphere.Radius;
			float n = sphere_centerLS.z - scene_bounding_sphere.Radius;
			float r = sphere_centerLS.x + scene_bounding_sphere.Radius;
			float t = sphere_centerLS.y + scene_bounding_sphere.Radius;
			float f = sphere_centerLS.z + scene_bounding_sphere.Radius;

			Matrix P = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

			BoundingBox::CreateFromPoints(cull_box, Vector4(l, b, n, 1.0f), Vector4(r, t, f, 1.0f));
			cull_box.Transform(cull_box, V.Invert());

			return { V,P };
		}

		std::array<Matrix, CASCADE_COUNT> RecalculateProjectionMatrices(Camera const& camera, float split_lambda, std::array<float, CASCADE_COUNT>& split_distances)
		{
			float camera_near = camera.Near();
			float camera_far = camera.Far();
			float fov = camera.Fov();
			float ar = camera.AspectRatio();
			float f = 1.0f / CASCADE_COUNT;
			for (uint32 i = 0; i < split_distances.size(); i++)
			{
				float fi = (i + 1) * f;
				float l = camera_near * pow(camera_far / camera_near, fi);
				float u = camera_near + (camera_far - camera_near) * fi;
				split_distances[i] = l * split_lambda + u * (1.0f - split_lambda);
			}

			std::array<Matrix, CASCADE_COUNT> projectionMatrices{};
			projectionMatrices[0] = XMMatrixPerspectiveFovLH(fov, ar, camera_near, split_distances[0]);
			for (uint32 i = 1; i < projectionMatrices.size(); ++i)
				projectionMatrices[i] = XMMatrixPerspectiveFovLH(fov, ar, split_distances[i - 1], split_distances[i]);
			return projectionMatrices;
		}

		float GaussianDistribution(float x, float sigma)
		{
			static const float square_root_of_two_pi = sqrt(pi_times_2<float>);
			return expf((-x * x) / (2 * sigma * sigma)) / (sigma * square_root_of_two_pi);
		}
		template<int16 N>
		std::array<float, 2 * N + 1> GaussKernel(float sigma)
		{
			std::array<float, 2 * N + 1> gauss{};
			float sum = 0.0f;
			for (int16 i = -N; i <= N; ++i)
			{
				gauss[i + N] = GaussianDistribution(i * 1.0f, sigma);
				sum += gauss[i + N];
			}
			for (int16 i = -N; i <= N; ++i)
			{
				gauss[i + N] /= sum;
			}

			return gauss;
		}
	}

	Renderer::Renderer(registry& reg, GfxDevice* gfx, uint32 width, uint32 height)
		: width(width), height(height), reg(reg), gfx(gfx), particle_renderer(gfx), picker(gfx)
	{
		g_GfxProfiler.Initialize(gfx->Device());
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

	void Renderer::Update(float dt)
	{
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
		ID3D11DeviceContext* context = gfx->Context();
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
		ID3D11DeviceContext* context = gfx->Context();
		if (renderer_settings.anti_aliasing & EAntiAliasing_FXAA)
		{
			fxaa_pass.Begin(context);
			PassToneMap();
			fxaa_pass.End(context);

			gfx->SetBackbuffer(); 
			PassFXAA();
		}
		else
		{
			gfx->SetBackbuffer();
			PassToneMap();
		}

	}
	void Renderer::ResolveToOffscreenFramebuffer()
	{
		ID3D11DeviceContext* context = gfx->Context();
		if (renderer_settings.anti_aliasing & EAntiAliasing_FXAA)
		{
			fxaa_pass.Begin(context);
			PassToneMap();
			fxaa_pass.End(context);
			
			offscreen_resolve_pass.Begin(context);
			PassFXAA();
			offscreen_resolve_pass.End(context);
		}
		else
		{
			offscreen_resolve_pass.Begin(context);
			PassToneMap();
			offscreen_resolve_pass.End(context);
		}
	}
	void Renderer::OnResize(uint32 w, uint32 h)
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
	void Renderer::NewFrame(Camera const* _camera)
	{
		BindGlobals();
		g_GfxProfiler.NewFrame();

		camera = _camera;
		frame_cbuf_data.global_ambient = Vector4{ renderer_settings.ambient_color[0], renderer_settings.ambient_color[1], renderer_settings.ambient_color[2], 1.0f };

		frame_cbuf_data.camera_near = camera->Near();
		frame_cbuf_data.camera_far = camera->Far();
		frame_cbuf_data.camera_position = Vector4(camera->Position());
		frame_cbuf_data.camera_forward = Vector4(camera->Forward());
		frame_cbuf_data.view = camera->View();
		frame_cbuf_data.projection = camera->Proj();
		frame_cbuf_data.viewprojection = camera->ViewProj();
		frame_cbuf_data.inverse_view = camera->View().Invert();
		frame_cbuf_data.inverse_projection = camera->Proj().Invert();
		frame_cbuf_data.inverse_view_projection = camera->ViewProj().Invert();
		frame_cbuf_data.screen_resolution_x = (float)width;
		frame_cbuf_data.screen_resolution_y = (float)height;
		frame_cbuf_data.mouse_normalized_coords_x = (current_scene_viewport.mouse_position_x - current_scene_viewport.scene_viewport_pos_x) / current_scene_viewport.scene_viewport_size_x;
		frame_cbuf_data.mouse_normalized_coords_y = (current_scene_viewport.mouse_position_y - current_scene_viewport.scene_viewport_pos_y) / current_scene_viewport.scene_viewport_size_y;

		frame_cbuffer->Update(gfx->Context(), frame_cbuf_data);

		//set for next frame
		frame_cbuf_data.previous_view = camera->View();
		frame_cbuf_data.previous_view = camera->Proj();
		frame_cbuf_data.previous_view_projection = camera->ViewProj(); 


		static float _near = 0.0f, far_plane = 0.0f, _fov = 0.0f, _ar = 0.0f;
		static constexpr float TOLERANCE = 1e-4f;
		if (fabs(_near - camera->Near()) > TOLERANCE || fabs(far_plane - camera->Far()) > TOLERANCE || fabs(_fov - camera->Fov()) > TOLERANCE || fabs(_ar - camera->AspectRatio()) > TOLERANCE)
		{
			_near = camera->Near();
			far_plane = camera->Far();
			_fov = camera->Fov();
			_ar = camera->AspectRatio();
			recreate_clusters = true;
		}
	}
	PickingData Renderer::GetLastPickingData() const
	{
		return last_picking_data;
	}
	std::vector<Timestamp> Renderer::GetProfilerResults()
	{
		return g_GfxProfiler.GetProfilingResults(gfx->Context());
	}

	void Renderer::LoadTextures()
	{
		TextureHandle tex_handle{};
		tex_handle = g_TextureManager.LoadTexture("Resources/Textures/lensflare/flare0.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		tex_handle = g_TextureManager.LoadTexture("Resources/Textures/lensflare/flare1.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		tex_handle = g_TextureManager.LoadTexture("Resources/Textures/lensflare/flare2.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		tex_handle = g_TextureManager.LoadTexture("Resources/Textures/lensflare/flare3.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		tex_handle = g_TextureManager.LoadTexture("Resources/Textures/lensflare/flare4.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		tex_handle = g_TextureManager.LoadTexture("Resources/Textures/lensflare/flare5.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		tex_handle = g_TextureManager.LoadTexture("Resources/Textures/lensflare/flare6.jpg");
		lens_flare_textures.push_back(g_TextureManager.GetTextureView(tex_handle));

		clouds_textures.resize(3);

		ID3D11Device* device = gfx->Device();
		ID3D11DeviceContext* context = gfx->Context();

		HRESULT hr = CreateDDSTextureFromFile(device, context, L"Resources\\Textures\\clouds\\weather.dds", nullptr, &clouds_textures[0]);
		GFX_CHECK_HR(hr);
		hr = CreateDDSTextureFromFile(device, context, L"Resources\\Textures\\clouds\\cloud.dds", nullptr, &clouds_textures[1]);
		GFX_CHECK_HR(hr);
		hr = CreateDDSTextureFromFile(device, context, L"Resources\\Textures\\clouds\\worley.dds", nullptr, &clouds_textures[2]);
		GFX_CHECK_HR(hr);

		foam_handle		= g_TextureManager.LoadTexture("Resources/Textures/foam.jpg");
		perlin_handle	= g_TextureManager.LoadTexture("Resources/Textures/perlin.dds");

		hex_bokeh_handle = g_TextureManager.LoadTexture("Resources/Textures/bokeh/Bokeh_Hex.dds");
		oct_bokeh_handle = g_TextureManager.LoadTexture("Resources/Textures/bokeh/Bokeh_Oct.dds");
		circle_bokeh_handle = g_TextureManager.LoadTexture("Resources/Textures/bokeh/Bokeh_Circle.dds");
		cross_bokeh_handle = g_TextureManager.LoadTexture("Resources/Textures/bokeh/Bokeh_Cross.dds");

	}
	void Renderer::CreateBuffers()
	{
		ID3D11Device* device = gfx->Device();

		frame_cbuffer = std::make_unique<GfxConstantBuffer<FrameCBuffer>>(device);
		light_cbuffer = std::make_unique<GfxConstantBuffer<LightCBuffer>>(device);
		object_cbuffer = std::make_unique<GfxConstantBuffer<ObjectCBuffer>>(device);
		material_cbuffer = std::make_unique<GfxConstantBuffer<MaterialCBuffer>>(device);
		shadow_cbuffer = std::make_unique<GfxConstantBuffer<ShadowCBuffer>>(device);
		postprocess_cbuffer = std::make_unique<GfxConstantBuffer<PostprocessCBuffer>>(device);
		compute_cbuffer = std::make_unique<GfxConstantBuffer<ComputeCBuffer>>(device);
		weather_cbuffer = std::make_unique<GfxConstantBuffer<WeatherCBuffer>>(device);
		voxel_cbuffer = std::make_unique<GfxConstantBuffer<VoxelCBuffer>>(device);
		terrain_cbuffer = std::make_unique<GfxConstantBuffer<TerrainCBuffer>>(device);

		GfxBufferDesc bokeh_indirect_draw_buffer_desc{};
		bokeh_indirect_draw_buffer_desc.size = 4 * sizeof(uint32);
		bokeh_indirect_draw_buffer_desc.misc_flags = GfxBufferMiscFlag::IndirectArgs;
		uint32 buffer_init[4] = { 0, 1, 0, 0 };
		bokeh_indirect_draw_buffer = std::make_unique<GfxBuffer>(gfx, bokeh_indirect_draw_buffer_desc, buffer_init);

		static constexpr uint32 CLUSTER_COUNT = CLUSTER_SIZE_X * CLUSTER_SIZE_Y * CLUSTER_SIZE_Z;
		voxels = std::make_unique<GfxBuffer>(gfx, StructuredBufferDesc<VoxelType>(VOXEL_RESOLUTION * VOXEL_RESOLUTION * VOXEL_RESOLUTION));
		clusters = std::make_unique<GfxBuffer>(gfx, StructuredBufferDesc<ClusterAABB>(CLUSTER_COUNT));
		light_counter = std::make_unique<GfxBuffer>(gfx, StructuredBufferDesc<uint32>(1));
		light_list = std::make_unique<GfxBuffer>(gfx, StructuredBufferDesc<uint32>(CLUSTER_COUNT * CLUSTER_MAX_LIGHTS));
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
		//for sky
		const SimpleVertex cube_vertices[8] = 
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

		const uint16 cube_indices[36] = 
		{
			// front
			0, 1, 2,
			2, 3, 0,
			// right
			1, 5, 6,
			6, 2, 1,
			// back
			7, 6, 5,
			5, 4, 7,
			// left
			4, 0, 3,
			3, 7, 4,
			// bottom
			4, 5, 1,
			1, 0, 4,
			// top
			3, 2, 6,
			6, 7, 3
		};

		cube_vb = std::make_unique<GfxBuffer>(gfx, VertexBufferDesc(ARRAYSIZE(cube_vertices), sizeof(SimpleVertex)), cube_vertices);
		cube_ib = std::make_unique<GfxBuffer>(gfx, IndexBufferDesc(ARRAYSIZE(cube_indices), true), cube_indices);

		const uint16 aabb_indices[] = {
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
		auto device = gfx->Device();

		D3D11_SAMPLER_DESC sampDesc = {};
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

		GFX_CHECK_HR(device->CreateSamplerState(&sampDesc, linear_wrap_sampler.GetAddressOf()));

		//point wrap sampler

		D3D11_SAMPLER_DESC pointWrapSampDesc = {};
		pointWrapSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		pointWrapSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		pointWrapSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		pointWrapSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		pointWrapSampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		pointWrapSampDesc.MinLOD = 0;
		pointWrapSampDesc.MaxLOD = D3D11_FLOAT32_MAX;

		GFX_CHECK_HR(device->CreateSamplerState(&pointWrapSampDesc, point_wrap_sampler.GetAddressOf()));

		//point border sampler
		D3D11_SAMPLER_DESC linearBorderSampDesc = {};
		linearBorderSampDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		linearBorderSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		linearBorderSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		linearBorderSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;

		linearBorderSampDesc.BorderColor[0] = 0.0f;
		linearBorderSampDesc.BorderColor[1] = 0.0f;
		linearBorderSampDesc.BorderColor[2] = 0.0f;
		linearBorderSampDesc.BorderColor[3] = 1e5f;

		linearBorderSampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		linearBorderSampDesc.MinLOD = 0;
		linearBorderSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		GFX_CHECK_HR(device->CreateSamplerState(&linearBorderSampDesc, linear_border_sampler.GetAddressOf()));

		//comparison sampler
		D3D11_SAMPLER_DESC comparisonSamplerDesc;
		ZeroMemory(&comparisonSamplerDesc, sizeof(D3D11_SAMPLER_DESC));
		comparisonSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		comparisonSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		comparisonSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		comparisonSamplerDesc.BorderColor[0] = 1.0f;
		comparisonSamplerDesc.BorderColor[1] = 1.0f;
		comparisonSamplerDesc.BorderColor[2] = 1.0f;
		comparisonSamplerDesc.BorderColor[3] = 1.0f;
		comparisonSamplerDesc.MinLOD = 0.f;
		comparisonSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		comparisonSamplerDesc.MipLODBias = 0.f;
		comparisonSamplerDesc.MaxAnisotropy = 0;
		comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
		comparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;

		GFX_CHECK_HR(device->CreateSamplerState(&comparisonSamplerDesc, shadow_sampler.GetAddressOf()));

		D3D11_SAMPLER_DESC linearClampSampDesc = {};
		linearClampSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		linearClampSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		linearClampSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		linearClampSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		linearClampSampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		linearClampSampDesc.MinLOD = 0;
		linearClampSampDesc.MaxLOD = D3D11_FLOAT32_MAX;

		GFX_CHECK_HR(device->CreateSamplerState(&linearClampSampDesc, linear_clamp_sampler.GetAddressOf()));

		D3D11_SAMPLER_DESC pointClampSampDesc = {};
		pointClampSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		pointClampSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		pointClampSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		pointClampSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		pointClampSampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		pointClampSampDesc.MinLOD = 0;
		pointClampSampDesc.MaxLOD = D3D11_FLOAT32_MAX;

		GFX_CHECK_HR(device->CreateSamplerState(&pointClampSampDesc, point_clamp_sampler.GetAddressOf()));

		D3D11_SAMPLER_DESC anisotropicSampDesc = {};
		anisotropicSampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		anisotropicSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		anisotropicSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		anisotropicSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		anisotropicSampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		anisotropicSampDesc.MinLOD = 0;
		anisotropicSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		anisotropicSampDesc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;

		GFX_CHECK_HR(device->CreateSamplerState(&anisotropicSampDesc, anisotropic_sampler.GetAddressOf()));
	}
	void Renderer::CreateRenderStates()
	{
		ID3D11Device* device = gfx->Device();

		CD3D11_BLEND_DESC bs_desc{ CD3D11_DEFAULT{} };
		bs_desc.RenderTarget[0].BlendEnable = true;
		bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		device->CreateBlendState(&bs_desc, additive_blend.GetAddressOf());

		bs_desc.RenderTarget[0].BlendEnable = true;
		bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

		//bs_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		//bs_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		//bs_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

		device->CreateBlendState(&bs_desc, alpha_blend.GetAddressOf());

		// Clear the blend state description.
		bs_desc = {};
		bs_desc.AlphaToCoverageEnable = FALSE;
		bs_desc.RenderTarget[0].BlendEnable = TRUE;
		bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bs_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bs_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		bs_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bs_desc.RenderTarget[0].RenderTargetWriteMask = 0x0f;
		device->CreateBlendState(&bs_desc, alpha_to_coverage.GetAddressOf());

		D3D11_DEPTH_STENCIL_DESC ds_desc = GfxCommonStates::DepthDefault();
		device->CreateDepthStencilState(&ds_desc, leq_depth.GetAddressOf());

		ds_desc = GfxCommonStates::DepthNone();
		device->CreateDepthStencilState(&ds_desc, no_depth_test.GetAddressOf());

		CD3D11_RASTERIZER_DESC rasterizer_state_desc(CD3D11_DEFAULT{});
		rasterizer_state_desc.ScissorEnable = TRUE;
		device->CreateRasterizerState(&rasterizer_state_desc, scissor_enabled.GetAddressOf());

		D3D11_RASTERIZER_DESC raster_desc = GfxCommonStates::CullNone();
		device->CreateRasterizerState(&raster_desc, cull_none.GetAddressOf());

		D3D11_RASTERIZER_DESC shadow_render_state_desc{};
		shadow_render_state_desc.CullMode = D3D11_CULL_FRONT;
		shadow_render_state_desc.FillMode = D3D11_FILL_SOLID;
		shadow_render_state_desc.DepthClipEnable = TRUE;
		shadow_render_state_desc.DepthBias = 8500; //maybe as parameter
		shadow_render_state_desc.DepthBiasClamp = 0.0f;
		shadow_render_state_desc.SlopeScaledDepthBias = 1.0f;

		device->CreateRasterizerState(&shadow_render_state_desc, shadow_depth_bias.GetAddressOf());

		D3D11_RASTERIZER_DESC wireframe_desc = GfxCommonStates::Wireframe();
		device->CreateRasterizerState(&wireframe_desc, wireframe.GetAddressOf());
	}

	void Renderer::CreateResolutionDependentResources(uint32 width, uint32 height)
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
		ID3D11Device* device = gfx->Device();
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
			for (uint32 i = 0; i < 6; ++i)
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
				dsv_desc.first_slice = (uint32)i;
				size_t j = shadow_cascade_maps->CreateDSV(&dsv_desc);
				ADRIA_ASSERT(j == i + 1);
			}
		}

		//ao random
		{
			std::vector<float> random_texture_data;
			random_texture_data.reserve(AO_NOISE_DIM * AO_NOISE_DIM * 4);
			RealRandomGenerator rand_float{ 0.0f, 1.0f };
			for (uint32 i = 0; i < ssao_kernel.size(); i++)
			{
				Vector4 offset(2 * rand_float() - 1, 2 * rand_float() - 1, rand_float(), 0.0f);
				offset.Normalize();
				offset *= rand_float();
				float scale = static_cast<float>(i) / ssao_kernel.size();
				scale = std::lerp(0.1f, 1.0f, scale * scale);
				offset *= scale;
				ssao_kernel[i] = offset;
			}

			for (int32 i = 0; i < AO_NOISE_DIM * AO_NOISE_DIM; i++)
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

			GfxTextureInitialData init_data{};
			init_data.pSysMem = (void*)random_texture_data.data();
			init_data.SysMemPitch = AO_NOISE_DIM * 4 * sizeof(float);
			ssao_random_texture = std::make_unique<GfxTexture>(gfx, random_tex_desc, &init_data);

			random_texture_data.clear();
			for (int32 i = 0; i < AO_NOISE_DIM * AO_NOISE_DIM; i++)
			{
				float rand = rand_float() * pi<float> *2.0f;
				random_texture_data.push_back(sin(rand));
				random_texture_data.push_back(cos(rand));
				random_texture_data.push_back(rand_float());
				random_texture_data.push_back(rand_float());
			}
			init_data.pSysMem = (void*)random_texture_data.data();
			hbao_random_texture = std::make_unique<GfxTexture>(gfx, random_tex_desc, &init_data);
		}

		//ocean
		{
			GfxTextureDesc desc{};
			desc.width = RESOLUTION;
			desc.height = RESOLUTION;
			desc.format = GfxFormat::R32_FLOAT;
			desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::UnorderedAccess;
			ocean_initial_spectrum = std::make_unique<GfxTexture>(gfx, desc);

			std::vector<float> ping_array(RESOLUTION * RESOLUTION);
			RealRandomGenerator rand_float{ 0.0f, 1.0f };
			for (size_t i = 0; i < ping_array.size(); ++i) ping_array[i] = rand_float() * 2.f * pi<float>;
			ping_pong_phase_textures[!pong_phase] = std::make_unique<GfxTexture>(gfx, desc);

			GfxTextureInitialData init_data{};
			init_data.pSysMem = ping_array.data();
			init_data.SysMemPitch = RESOLUTION * sizeof(float);
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

	void Renderer::CreateBokehViews(uint32 width, uint32 height)
	{
		ID3D11Device* device = gfx->Device();

		uint32 const max_bokeh = width * height;

		GfxBufferDesc bokeh_buffer_desc{};
		bokeh_buffer_desc.stride = 8 * sizeof(float);
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
	void Renderer::CreateRenderTargets(uint32 width, uint32 height)
	{
		ID3D11Device* device = gfx->Device();

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
	void Renderer::CreateGBuffer(uint32 width, uint32 height)
	{
		gbuffer.clear();
		
		GfxTextureDesc render_target_desc{};
		render_target_desc.width = width;
		render_target_desc.height = height;
		render_target_desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::RenderTarget;
		
		for (uint32 i = 0; i < EGBufferSlot_Count; ++i)
		{
			render_target_desc.format = GBUFFER_FORMAT[i];
			gbuffer.push_back(std::make_unique<GfxTexture>(gfx, render_target_desc));
		}
	}
	void Renderer::CreateAOTexture(uint32 width, uint32 height)
	{
		GfxTextureDesc ao_tex_desc{};
		ao_tex_desc.width = width;
		ao_tex_desc.height = height;
		ao_tex_desc.format = GfxFormat::R8_UNORM;
		ao_tex_desc.bind_flags = GfxBindFlag::ShaderResource | GfxBindFlag::RenderTarget;
		ao_texture = std::make_unique<GfxTexture>(gfx, ao_tex_desc);
	}
	void Renderer::CreateRenderPasses(uint32 width, uint32 height)
	{
		static constexpr float clear_black[4] = {0.0f,0.0f,0.0f,0.0f};

		GfxColorAttachmentDesc gbuffer_normal_attachment{};
		gbuffer_normal_attachment.view = gbuffer[EGBufferSlot_NormalMetallic]->RTV();
		gbuffer_normal_attachment.clear_color = GfxClearValue(clear_black);
		gbuffer_normal_attachment.load_op = GfxLoadAccessOp::Clear;

		GfxColorAttachmentDesc gbuffer_albedo_attachment{};
		gbuffer_albedo_attachment.view = gbuffer[EGBufferSlot_DiffuseRoughness]->RTV();
		gbuffer_albedo_attachment.clear_color = GfxClearValue(clear_black);
		gbuffer_albedo_attachment.load_op = GfxLoadAccessOp::Clear;

		GfxColorAttachmentDesc gbuffer_emissive_attachment{};
		gbuffer_emissive_attachment.view = gbuffer[EGBufferSlot_Emissive]->RTV();
		gbuffer_emissive_attachment.clear_color = GfxClearValue(clear_black);
		gbuffer_emissive_attachment.load_op = GfxLoadAccessOp::Clear;

		GfxColorAttachmentDesc decal_normal_attachment{};
		decal_normal_attachment.view = gbuffer[EGBufferSlot_NormalMetallic]->RTV();
		decal_normal_attachment.load_op = GfxLoadAccessOp::Load;

		GfxColorAttachmentDesc decal_albedo_attachment{};
		decal_albedo_attachment.view = gbuffer[EGBufferSlot_DiffuseRoughness]->RTV();
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
			gbuffer_pass = GfxRenderPass(render_pass_desc);
		}

		//ambient pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(hdr_color_clear_attachment);
			ambient_pass = GfxRenderPass(render_pass_desc);
		}

		//lighting pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(hdr_color_load_attachment);

			lighting_pass = GfxRenderPass(render_pass_desc);
		}

		//forward pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(hdr_color_load_attachment);
			render_pass_desc.dsv_attachment = depth_load_attachment;
			forward_pass = GfxRenderPass(render_pass_desc);
		}

		//particle pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(hdr_color_load_attachment);
			particle_pass = GfxRenderPass(render_pass_desc);
		}

		//decal pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(decal_albedo_attachment);
			render_pass_desc.rtv_attachments.push_back(decal_normal_attachment);
			decal_pass = GfxRenderPass(render_pass_desc);
		}

		//fxaa pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(fxaa_source_attachment);
			fxaa_pass = GfxRenderPass(render_pass_desc);
		}

		//shadow map pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = SHADOW_MAP_SIZE;
			render_pass_desc.height = SHADOW_MAP_SIZE;
			render_pass_desc.dsv_attachment = shadow_map_attachment;
			shadow_map_pass = GfxRenderPass(render_pass_desc);
		}

		//shadow cubemap pass
		{
			for (uint32 i = 0; i < 6; ++i)
			{
				GfxDepthAttachmentDesc shadow_cubemap_attachment{};
				shadow_cubemap_attachment.view = shadow_depth_cubemap->DSV(i + 1);
				shadow_cubemap_attachment.clear_depth = GfxClearValue(1.0f, 0);
				shadow_cubemap_attachment.load_op = GfxLoadAccessOp::Clear;

				GfxRenderPassDesc render_pass_desc{};
				render_pass_desc.width = SHADOW_CUBE_SIZE;
				render_pass_desc.height = SHADOW_CUBE_SIZE;
				render_pass_desc.dsv_attachment = shadow_cubemap_attachment;
				shadow_cubemap_pass[i] = GfxRenderPass(render_pass_desc);
			}
		}

		//cascade shadow pass
		{
			cascade_shadow_pass.clear();
			for (uint32 i = 0; i < CASCADE_COUNT; ++i)
			{
				GfxDepthAttachmentDesc cascade_shadow_map_attachment{};
				cascade_shadow_map_attachment.view = shadow_cascade_maps->DSV(i + 1);
				cascade_shadow_map_attachment.clear_depth = GfxClearValue(1.0f, 0);
				cascade_shadow_map_attachment.load_op = GfxLoadAccessOp::Clear;

				GfxRenderPassDesc render_pass_desc{};
				render_pass_desc.width = SHADOW_CASCADE_SIZE;
				render_pass_desc.height = SHADOW_CASCADE_SIZE;
				render_pass_desc.dsv_attachment = cascade_shadow_map_attachment;
				cascade_shadow_pass.push_back(GfxRenderPass(render_pass_desc));
			}
		}

		//ssao pass
		{

			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(ssao_attachment);
			ssao_pass = GfxRenderPass(render_pass_desc);
			hbao_pass = ssao_pass;
		}

		//ping pong passes
		{
			
			GfxRenderPassDesc ping_render_pass_desc{};
			ping_render_pass_desc.width = width;
			ping_render_pass_desc.height = height;
			ping_render_pass_desc.rtv_attachments.push_back(ping_color_load_attachment);
			postprocess_passes[0] = GfxRenderPass(ping_render_pass_desc);

			GfxRenderPassDesc pong_render_pass_desc{};
			pong_render_pass_desc.width = width;
			pong_render_pass_desc.height = height;
			pong_render_pass_desc.rtv_attachments.push_back(pong_color_load_attachment);
			postprocess_passes[1] = GfxRenderPass(pong_render_pass_desc);

		}

		//voxel debug pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(hdr_color_clear_attachment);
			render_pass_desc.dsv_attachment = depth_load_attachment;
			voxel_debug_pass = GfxRenderPass(render_pass_desc);
		}

		//offscreen_resolve_pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(offscreen_clear_attachment);
			offscreen_resolve_pass = GfxRenderPass(render_pass_desc);
		}

		//velocity buffer pass
		{
			GfxRenderPassDesc render_pass_desc{};
			render_pass_desc.width = width;
			render_pass_desc.height = height;
			render_pass_desc.rtv_attachments.push_back(velocity_clear_attachment);
			velocity_buffer_pass = GfxRenderPass(render_pass_desc);
		}
	}
	void Renderer::CreateComputeTextures(uint32 width, uint32 height)
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
		auto device = gfx->Device();
		auto context = gfx->Context();

		auto skyboxes = reg.view<Skybox>();
		ID3D11ShaderResourceView* unfiltered_env_srv = nullptr;
		for (auto skybox : skyboxes)
		{
			auto const& _skybox = skyboxes.get(skybox);
			unfiltered_env_srv = g_TextureManager.GetTextureView(_skybox.cubemap_texture);
			if (unfiltered_env_srv) break;
		}

		ArcPtr<ID3D11Texture2D> unfiltered_env_tex = nullptr;
		unfiltered_env_srv->GetResource(reinterpret_cast<ID3D11Resource**>(unfiltered_env_tex.GetAddressOf()));

		D3D11_TEXTURE2D_DESC tex_desc{};
		unfiltered_env_tex->GetDesc(&tex_desc);
		//Compute pre-filtered specular environment map.
		{
			GfxShaderBytecode cs_blob;
			GfxShaderCompiler::GetBytecodeFromCompiledShader("Resources/Compiled Shaders/SpmapCS.cso", cs_blob);
			GfxComputeShader spmap_program{ device, cs_blob };

			ArcPtr<ID3D11Texture2D> env_tex = nullptr;
			D3D11_TEXTURE2D_DESC env_desc = {};
			env_desc.Width = tex_desc.Width;
			env_desc.Height = tex_desc.Height;
			env_desc.MipLevels = 0;
			env_desc.ArraySize = 6;
			env_desc.Format = tex_desc.Format;
			env_desc.SampleDesc.Count = 1;
			env_desc.Usage = D3D11_USAGE_DEFAULT;
			env_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
			env_desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
			GFX_CHECK_HR(device->CreateTexture2D(&env_desc, nullptr, env_tex.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC env_srv_desc = {};
			env_srv_desc.Format = env_desc.Format;
			env_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			env_srv_desc.TextureCube.MostDetailedMip = 0;
			env_srv_desc.TextureCube.MipLevels = -1;
			GFX_CHECK_HR(device->CreateShaderResourceView(env_tex.Get(), &env_srv_desc, env_srv.GetAddressOf()));

			//Copy 0th mipmap level into destination environment map.
			for (uint32 arraySlice = 0; arraySlice < 6; ++arraySlice)
			{
				const uint32 subresourceIndex = D3D11CalcSubresource(0, arraySlice, tex_desc.MipLevels);
				context->CopySubresourceRegion(env_tex.Get(), subresourceIndex, 0, 0, 0, unfiltered_env_tex.Get(), subresourceIndex, nullptr);
			}

			DECLSPEC_ALIGN(16) struct RoughnessCBuffer
			{
				float roughness;
				float padding[3];
			};

			GfxConstantBuffer<RoughnessCBuffer> roughness_cb(device);
			spmap_program.Bind(context);
			roughness_cb.Bind(context, GfxShaderStage::CS, 0);
			context->CSSetShaderResources(0, 1, &unfiltered_env_srv);

			float const delta_roughness = 1.0f / (std::max)(float(tex_desc.MipLevels - 1), 1.0f);

			uint32 size = (std::max)(tex_desc.Width, tex_desc.Height);
			RoughnessCBuffer spmap_constants{};

			ArcPtr<ID3D11UnorderedAccessView> env_uav;
			D3D11_UNORDERED_ACCESS_VIEW_DESC env_uav_desc{};
			env_uav_desc.Format = env_desc.Format;
			env_uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;

			env_uav_desc.Texture2DArray.FirstArraySlice = 0;
			env_uav_desc.Texture2DArray.ArraySize = env_desc.ArraySize;

			for (uint32 level = 1; level < tex_desc.MipLevels; ++level, size /= 2)
			{
				const uint32 numGroups = std::max<uint32>(1, size / 32);

				env_uav_desc.Texture2DArray.MipSlice = level;

				GFX_CHECK_HR(device->CreateUnorderedAccessView(env_tex.Get(), &env_uav_desc, env_uav.GetAddressOf()));

				spmap_constants = { level * delta_roughness };

				roughness_cb.Update(context, spmap_constants);
				context->CSSetUnorderedAccessViews(0, 1, env_uav.GetAddressOf(), nullptr);
				context->Dispatch(numGroups, numGroups, 6);
			}

			ID3D11Buffer* null_buffer[] = { nullptr };
			context->CSSetConstantBuffers(0, 1, null_buffer);
			ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
			context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
		}
		// Compute diffuse irradiance cubemap.
		{
			GfxShaderBytecode cs_blob;
			GfxShaderCompiler::GetBytecodeFromCompiledShader("Resources/Compiled Shaders/IrmapCS.cso", cs_blob);
			GfxComputeShader irmap_program(device, cs_blob);

			ArcPtr<ID3D11Texture2D> irmap_tex = nullptr;
			D3D11_TEXTURE2D_DESC irmap_desc{};
			irmap_desc.Width = 32;
			irmap_desc.Height = 32;
			irmap_desc.MipLevels = 1;
			irmap_desc.ArraySize = 6;
			irmap_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			irmap_desc.SampleDesc.Count = 1;
			irmap_desc.Usage = D3D11_USAGE_DEFAULT;
			irmap_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			irmap_desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
			GFX_CHECK_HR(device->CreateTexture2D(&irmap_desc, nullptr, irmap_tex.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC irmap_srv_desc{};
			irmap_srv_desc.Format = irmap_desc.Format;
			irmap_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			irmap_srv_desc.TextureCube.MostDetailedMip = 0;
			irmap_srv_desc.TextureCube.MipLevels = -1;
			GFX_CHECK_HR(device->CreateShaderResourceView(irmap_tex.Get(), &irmap_srv_desc, irmap_srv.GetAddressOf()));

			ArcPtr<ID3D11UnorderedAccessView> irmap_uav;

			D3D11_UNORDERED_ACCESS_VIEW_DESC irmap_uav_desc{};
			irmap_uav_desc.Format = irmap_desc.Format;
			irmap_uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			irmap_uav_desc.Texture2DArray.MipSlice = 0;
			irmap_uav_desc.Texture2DArray.FirstArraySlice = 0;
			irmap_uav_desc.Texture2DArray.ArraySize = irmap_desc.ArraySize;

			GFX_CHECK_HR(device->CreateUnorderedAccessView(irmap_tex.Get(), &irmap_uav_desc, irmap_uav.GetAddressOf()));

			context->CSSetShaderResources(0, 1, env_srv.GetAddressOf());
			context->CSSetUnorderedAccessViews(0, 1, irmap_uav.GetAddressOf(), nullptr);

			irmap_program.Bind(context);
			context->Dispatch(irmap_desc.Width / 32, irmap_desc.Height / 32, 6);
			ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
			context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
		}

		// Compute Cook-Torrance BRDF 2D LUT for split-sum approximation.
		{
			GfxShaderBytecode cs_blob;
			GfxShaderCompiler::GetBytecodeFromCompiledShader("Resources/Compiled Shaders/SpbrdfCS.cso", cs_blob);
			GfxComputeShader BRDFprogram(device, cs_blob);

			ArcPtr<ID3D11Texture2D> brdf_tex = nullptr;
			D3D11_TEXTURE2D_DESC brdf_desc = {};
			brdf_desc.Width = 256;
			brdf_desc.Height = 256;
			brdf_desc.MipLevels = 1;
			brdf_desc.ArraySize = 1;
			brdf_desc.Format = DXGI_FORMAT_R16G16_FLOAT;
			brdf_desc.SampleDesc.Count = 1;
			brdf_desc.Usage = D3D11_USAGE_DEFAULT;
			brdf_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

			GFX_CHECK_HR(device->CreateTexture2D(&brdf_desc, nullptr, brdf_tex.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC brdf_srv_desc = {};
			brdf_srv_desc.Format = brdf_desc.Format;
			brdf_srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			brdf_srv_desc.Texture2D.MostDetailedMip = 0;
			brdf_srv_desc.Texture2D.MipLevels = -1;
			GFX_CHECK_HR(device->CreateShaderResourceView(brdf_tex.Get(), &brdf_srv_desc, brdf_srv.GetAddressOf()));

			ArcPtr<ID3D11UnorderedAccessView> brdf_uav;

			D3D11_UNORDERED_ACCESS_VIEW_DESC brdf_uav_desc = {};
			brdf_uav_desc.Format = brdf_desc.Format;
			brdf_uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			brdf_uav_desc.Texture2D.MipSlice = 0;

			GFX_CHECK_HR(device->CreateUnorderedAccessView(brdf_tex.Get(), &brdf_uav_desc, brdf_uav.GetAddressOf()));

			context->CSSetUnorderedAccessViews(0, 1, brdf_uav.GetAddressOf(), nullptr);

			BRDFprogram.Bind(context);
			context->Dispatch(brdf_desc.Width / 32, brdf_desc.Height / 32, 1);
			ID3D11UnorderedAccessView* nullUAV[] = { nullptr };
			context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
		}

		ibl_textures_generated = true;
	}

	///////////////////////////////////////////////////////////////////////

	void Renderer::BindGlobals()
	{
		static bool called = false;

		if (!called)
		{
			ID3D11DeviceContext* context = gfx->Context();
			//VS GLOBALS
			frame_cbuffer->Bind(context,  GfxShaderStage::VS, CBUFFER_SLOT_FRAME);
			object_cbuffer->Bind(context, GfxShaderStage::VS, CBUFFER_SLOT_OBJECT);
			shadow_cbuffer->Bind(context, GfxShaderStage::VS, CBUFFER_SLOT_SHADOW);
			weather_cbuffer->Bind(context, GfxShaderStage::VS, CBUFFER_SLOT_WEATHER);
			voxel_cbuffer->Bind(context,  GfxShaderStage::VS, CBUFFER_SLOT_VOXEL);
			context->VSSetSamplers(0, 1, linear_wrap_sampler.GetAddressOf());

			//TS/HS GLOBALS
			frame_cbuffer->Bind(context, GfxShaderStage::DS, CBUFFER_SLOT_FRAME);
			frame_cbuffer->Bind(context, GfxShaderStage::HS, CBUFFER_SLOT_FRAME);
			context->DSSetSamplers(0, 1, linear_wrap_sampler.GetAddressOf());
			context->HSSetSamplers(0, 1, linear_wrap_sampler.GetAddressOf());

			//CS GLOBALS
			frame_cbuffer->Bind(context, GfxShaderStage::CS, CBUFFER_SLOT_FRAME);
			compute_cbuffer->Bind(context, GfxShaderStage::CS, CBUFFER_SLOT_COMPUTE);
			voxel_cbuffer->Bind(context, GfxShaderStage::CS, CBUFFER_SLOT_VOXEL);
			context->CSSetSamplers(0, 1, linear_wrap_sampler.GetAddressOf());
			context->CSSetSamplers(1, 1, point_wrap_sampler.GetAddressOf());
			context->CSSetSamplers(2, 1, linear_border_sampler.GetAddressOf());
			context->CSSetSamplers(3, 1, linear_clamp_sampler.GetAddressOf());

			//GS GLOBALS
			frame_cbuffer->Bind(context, GfxShaderStage::GS, CBUFFER_SLOT_FRAME);
			light_cbuffer->Bind(context, GfxShaderStage::GS, CBUFFER_SLOT_LIGHT);
			voxel_cbuffer->Bind(context, GfxShaderStage::GS, CBUFFER_SLOT_VOXEL);
			context->GSSetSamplers(1, 1, point_wrap_sampler.GetAddressOf());
			context->GSSetSamplers(4, 1, point_clamp_sampler.GetAddressOf());
			
			//PS GLOBALS
			material_cbuffer->Bind(context, GfxShaderStage::PS, CBUFFER_SLOT_MATERIAL);
			frame_cbuffer->Bind(context, GfxShaderStage::PS, CBUFFER_SLOT_FRAME);
			light_cbuffer->Bind(context, GfxShaderStage::PS, CBUFFER_SLOT_LIGHT);
			shadow_cbuffer->Bind(context, GfxShaderStage::PS, CBUFFER_SLOT_SHADOW);
			postprocess_cbuffer->Bind(context, GfxShaderStage::PS, CBUFFER_SLOT_POSTPROCESS);
			weather_cbuffer->Bind(context, GfxShaderStage::PS, CBUFFER_SLOT_WEATHER);
			voxel_cbuffer->Bind(context, GfxShaderStage::PS, CBUFFER_SLOT_VOXEL);
			terrain_cbuffer->Bind(context, GfxShaderStage::PS, CBUFFER_SLOT_TERRAIN);
			context->PSSetSamplers(0, 1, linear_wrap_sampler.GetAddressOf());
			context->PSSetSamplers(1, 1, point_wrap_sampler.GetAddressOf());
			context->PSSetSamplers(2, 1, linear_border_sampler.GetAddressOf());
			context->PSSetSamplers(3, 1, linear_clamp_sampler.GetAddressOf());
			context->PSSetSamplers(4, 1, point_clamp_sampler.GetAddressOf());
			context->PSSetSamplers(5, 1, shadow_sampler.GetAddressOf());
			context->PSSetSamplers(6, 1, anisotropic_sampler.GetAddressOf());
			called = true;
		}

	}

	void Renderer::UpdateCBuffers(float dt)
	{
		compute_cbuf_data.bokeh_blur_threshold = renderer_settings.bokeh_blur_threshold;
		compute_cbuf_data.bokeh_lum_threshold = renderer_settings.bokeh_lum_threshold;
		compute_cbuf_data.dof_params = XMVectorSet(renderer_settings.dof_near_blur, renderer_settings.dof_near, renderer_settings.dof_far, renderer_settings.dof_far_blur);
		compute_cbuf_data.bokeh_radius_scale = renderer_settings.bokeh_radius_scale;
		compute_cbuf_data.bokeh_color_scale = renderer_settings.bokeh_color_scale;
		compute_cbuf_data.bokeh_fallout = renderer_settings.bokeh_fallout;

		compute_cbuf_data.visualize_tiled = static_cast<int32>(renderer_settings.visualize_tiled);
		compute_cbuf_data.visualize_max_lights = renderer_settings.visualize_max_lights;

		compute_cbuf_data.threshold = renderer_settings.bloom_threshold;
		compute_cbuf_data.bloom_scale = renderer_settings.bloom_scale;

		std::array<float, 9> coeffs{};
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

		ID3D11DeviceContext* context = gfx->Context();
		compute_cbuffer->Update(context, compute_cbuf_data);
	}
	void Renderer::UpdateOcean(float dt)
	{
		ID3D11DeviceContext* context = gfx->Context();
		if (reg.size<Ocean>() == 0) return;
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Ocean Update Pass");
		
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
			ShaderManager::GetShaderProgram(ShaderProgram::OceanInitialSpectrum)->Bind(context);

			static ID3D11UnorderedAccessView* null_uav = nullptr;

			ID3D11UnorderedAccessView* uav[] = { ocean_initial_spectrum->UAV() };

			context->CSSetUnorderedAccessViews(0, 1, uav, nullptr);

			context->Dispatch(RESOLUTION / 32, RESOLUTION / 32, 1);

			context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);

			renderer_settings.recreate_initial_spectrum = false;
		}

		//phase
		{
			static ID3D11ShaderResourceView* null_srv = nullptr;
			static ID3D11UnorderedAccessView* null_uav = nullptr;

			ID3D11ShaderResourceView*  srv[] = { ping_pong_phase_textures[pong_phase]->SRV()};
			ID3D11UnorderedAccessView* uav[] = { ping_pong_phase_textures[!pong_phase]->UAV() };

			ShaderManager::GetShaderProgram(ShaderProgram::OceanPhase)->Bind(context);

			context->CSSetShaderResources(0, 1, srv);
			context->CSSetUnorderedAccessViews(0, 1, uav, nullptr);

			context->Dispatch(RESOLUTION / 32, RESOLUTION / 32, 1);

			context->CSSetShaderResources(0, 1, &null_srv);
			context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);
			pong_phase = !pong_phase;
		}

		//spectrum
		{
			ID3D11ShaderResourceView* srvs[]	= { ping_pong_phase_textures[pong_phase]->SRV(), ocean_initial_spectrum->SRV() };
			ID3D11UnorderedAccessView* uav[]	= { ping_pong_spectrum_textures[pong_spectrum]->UAV() };
			static ID3D11ShaderResourceView* null_srv = nullptr;
			static ID3D11UnorderedAccessView* null_uav = nullptr;

			ShaderManager::GetShaderProgram(ShaderProgram::OceanSpectrum)->Bind(context);

			context->CSSetShaderResources(0, ARRAYSIZE(srvs), srvs);
			context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uav), uav, nullptr);

			context->Dispatch(RESOLUTION / 32, RESOLUTION / 32, 1);

			context->CSSetShaderResources(0, 1, &null_srv);
			context->CSSetShaderResources(1, 1, &null_srv);
			context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);

			pong_spectrum = !pong_spectrum;
		}

		//fft
		{
			struct FFTCBuffer
			{
				uint32 seq_count;
				uint32 subseq_count;
			};
			static FFTCBuffer fft_cbuf_data{ .seq_count = RESOLUTION };
			static GfxConstantBuffer<FFTCBuffer> fft_cbuffer(gfx->Device());

			fft_cbuffer.Bind(context, GfxShaderStage::CS, 10);
			//fft horizontal
			{
				ShaderManager::GetShaderProgram(ShaderProgram::OceanFFT_Horizontal)->Bind(context);
				for (uint32 p = 1; p < RESOLUTION; p <<= 1)
				{

					ID3D11ShaderResourceView* srv[]	= { ping_pong_spectrum_textures[!pong_spectrum]->SRV()};
					ID3D11UnorderedAccessView* uav[]= { ping_pong_spectrum_textures[pong_spectrum]->UAV() };

					static ID3D11ShaderResourceView* null_srv = nullptr;
					static ID3D11UnorderedAccessView* null_uav = nullptr;

					fft_cbuf_data.subseq_count = p;
					fft_cbuffer.Update(context, fft_cbuf_data);

					context->CSSetShaderResources(0, 1, srv);
					context->CSSetUnorderedAccessViews(0, 1, uav, nullptr);

					context->Dispatch(RESOLUTION, 1, 1);

					context->CSSetShaderResources(0, 1, &null_srv);
					context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);
					pong_spectrum = !pong_spectrum;
				}
			}
			//fft vertical
			{
				ShaderManager::GetShaderProgram(ShaderProgram::OceanFFT_Vertical)->Bind(context);
				for (uint32 p = 1; p < RESOLUTION; p <<= 1)
				{
					ID3D11ShaderResourceView* srv[] = { ping_pong_spectrum_textures[!pong_spectrum]->SRV() };
					ID3D11UnorderedAccessView* uav[] = { ping_pong_spectrum_textures[pong_spectrum]->UAV() };

					static ID3D11ShaderResourceView* null_srv = nullptr;
					static ID3D11UnorderedAccessView* null_uav = nullptr;

					fft_cbuf_data.subseq_count = p;
					fft_cbuffer.Update(context, fft_cbuf_data);

					context->CSSetShaderResources(0, 1, srv);
					context->CSSetUnorderedAccessViews(0, 1, uav, nullptr);

					context->Dispatch(RESOLUTION, 1, 1);

					context->CSSetShaderResources(0, 1, &null_srv);
					context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);
					pong_spectrum = !pong_spectrum;
				}
			}
		}
		//normal
		{
			static ID3D11ShaderResourceView* null_srv = nullptr;
			static ID3D11UnorderedAccessView* null_uav = nullptr;
			ShaderManager::GetShaderProgram(ShaderProgram::OceanNormalMap)->Bind(context);

			ID3D11ShaderResourceView* final_spectrum = ping_pong_spectrum_textures[!pong_spectrum]->SRV();
			ID3D11UnorderedAccessView* normal_map_uav = ocean_normal_map->UAV();

			context->CSSetShaderResources(0, 1, &final_spectrum);
			context->CSSetUnorderedAccessViews(0, 1, &normal_map_uav, nullptr);

			context->Dispatch(RESOLUTION / 32, RESOLUTION / 32, 1);

			context->CSSetShaderResources(0, 1, &null_srv);
			context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);
		}
	}
	void Renderer::UpdateWeather(float dt)
	{
		ID3D11DeviceContext* context = gfx->Context();

		static float total_time = 0.0f;
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

		weather_cbuffer->Update(context, weather_cbuf_data);
	}
	void Renderer::UpdateParticles(float dt)
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
		uint32 current_light_count = (uint32)reg.size<Light>();
		static uint32 light_count = 0;
		bool light_count_changed = current_light_count != light_count;
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
			light_data.position  = XMVector4Transform(light.position, camera->View());
			light_data.direction = XMVector4Transform(light.direction, camera->View());
			light_data.range = light.range;
			light_data.type = static_cast<int>(light.type);
			light_data.inner_cosine = light.inner_cosine;
			light_data.outer_cosine = light.outer_cosine;
			light_data.active = light.active;
			light_data.casts_shadows = light.casts_shadows;
			
			lights_data.push_back(light_data);
		}
		lights->Update(lights_data);

	}
	void Renderer::UpdateTerrainData()
	{
		ID3D11DeviceContext* context = gfx->Context();
		terrain_cbuf_data.texture_scale = TerrainComponent::texture_scale;
		terrain_cbuf_data.ocean_active = reg.size<Ocean>() != 0;
		terrain_cbuffer->Update(context, terrain_cbuf_data);
	}
	void Renderer::UpdateVoxelData()
	{
		float const f = 0.05f / renderer_settings.voxel_size;

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

		voxel_cbuffer->Update(gfx->Context(), voxel_cbuf_data);
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

	///////////////////////////////////////////////////////////////////////

	void Renderer::PassPicking()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(pick_in_current_frame);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Picking Pass");
		pick_in_current_frame = false;
		last_picking_data = picker.Pick(depth_target->SRV(), gbuffer[EGBufferSlot_NormalMetallic]->SRV());
	}
	void Renderer::PassGBuffer()
	{
		ID3D11DeviceContext* context = gfx->Context();
		AdriaGfxProfileCondScope(context, "GBuffer Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"GBuffer Pass");

		std::vector<ID3D11ShaderResourceView*> nullSRVs(gbuffer.size() + 1, nullptr);
		context->PSSetShaderResources(0, static_cast<uint32>(nullSRVs.size()), nullSRVs.data());

		struct BatchParams
		{
			ShaderProgram shader_program;
			bool double_sided;
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
		
		gbuffer_pass.Begin(context);
		{
			for (auto const& [params, entities] : batched_entities)
			{
				ShaderManager::GetShaderProgram(params.shader_program)->Bind(context);
				if (params.double_sided) context->RSSetState(cull_none.Get());
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
					object_cbuffer->Update(context, object_cbuf_data);

					material_cbuf_data.albedo_factor = material.albedo_factor;
					material_cbuf_data.metallic_factor = material.metallic_factor;
					material_cbuf_data.roughness_factor = material.roughness_factor;
					material_cbuf_data.emissive_factor = material.emissive_factor;
					material_cbuf_data.alpha_cutoff = material.alpha_cutoff;
					material_cbuffer->Update(context, material_cbuf_data);

					static ID3D11ShaderResourceView* const null_view = nullptr;

					if (material.albedo_texture != INVALID_TEXTURE_HANDLE)
					{
						auto view = g_TextureManager.GetTextureView(material.albedo_texture);
						context->PSSetShaderResources(TEXTURE_SLOT_DIFFUSE, 1, &view);
					}

					if (material.metallic_roughness_texture != INVALID_TEXTURE_HANDLE)
					{
						auto view = g_TextureManager.GetTextureView(material.metallic_roughness_texture);
						context->PSSetShaderResources(TEXTURE_SLOT_ROUGHNESS_METALLIC, 1, &view);
					}
					else
					{
						context->PSSetShaderResources(TEXTURE_SLOT_ROUGHNESS_METALLIC, 1, &null_view);
					}

					if (material.normal_texture != INVALID_TEXTURE_HANDLE)
					{
						auto view = g_TextureManager.GetTextureView(material.normal_texture);
						context->PSSetShaderResources(TEXTURE_SLOT_NORMAL, 1, &view);
					}
					else
					{
						context->PSSetShaderResources(TEXTURE_SLOT_NORMAL, 1, &null_view);
					}

					if (material.emissive_texture != INVALID_TEXTURE_HANDLE)
					{
						auto view = g_TextureManager.GetTextureView(material.emissive_texture);
						context->PSSetShaderResources(TEXTURE_SLOT_EMISSIVE, 1, &view);
					}
					else
					{
						context->PSSetShaderResources(TEXTURE_SLOT_EMISSIVE, 1, &null_view);
					}

					mesh.Draw(context);
				}
				if (params.double_sided) context->RSSetState(nullptr); 
			}
			
			auto terrain_view = reg.view<Mesh, Transform, AABB, TerrainComponent>();
			ShaderManager::GetShaderProgram(ShaderProgram::GBuffer_Terrain)->Bind(context);
			for (auto e : terrain_view)
			{
				auto [mesh, transform, aabb, terrain] = terrain_view.get<Mesh, Transform, AABB, TerrainComponent>(e);

				if (!aabb.camera_visible) continue;

				object_cbuf_data.model = transform.current_transform;
				object_cbuf_data.transposed_inverse_model = XMMatrixTranspose(XMMatrixInverse(nullptr, object_cbuf_data.model));
				object_cbuffer->Update(context, object_cbuf_data);

				if (terrain.grass_texture != INVALID_TEXTURE_HANDLE)
				{
					auto view = g_TextureManager.GetTextureView(terrain.grass_texture);
					context->PSSetShaderResources(TEXTURE_SLOT_GRASS, 1, &view);
				}
				if (terrain.base_texture != INVALID_TEXTURE_HANDLE)
				{
					auto view = g_TextureManager.GetTextureView(terrain.base_texture);
					context->PSSetShaderResources(TEXTURE_SLOT_BASE, 1, &view);
				}
				if (terrain.rock_texture != INVALID_TEXTURE_HANDLE)
				{
					auto view = g_TextureManager.GetTextureView(terrain.rock_texture);
					context->PSSetShaderResources(TEXTURE_SLOT_ROCK, 1, &view);
				}
				if (terrain.sand_texture != INVALID_TEXTURE_HANDLE)
				{
					auto view = g_TextureManager.GetTextureView(terrain.sand_texture);
					context->PSSetShaderResources(TEXTURE_SLOT_SAND, 1, &view);
				}

				if (terrain.layer_texture != INVALID_TEXTURE_HANDLE)
				{
					auto view = g_TextureManager.GetTextureView(terrain.layer_texture);
					context->PSSetShaderResources(TEXTURE_SLOT_LAYER, 1, &view);
				}
				mesh.Draw(context);
			}

			auto foliage_view = reg.view<Mesh, Transform, Material, AABB, Foliage>();
			ShaderManager::GetShaderProgram(ShaderProgram::GBuffer_Foliage)->Bind(context);
			for (auto e : foliage_view)
			{
				auto [mesh, transform, aabb, material] = foliage_view.get<Mesh, Transform, AABB, Material>(e);
				if (!aabb.camera_visible) continue;

				object_cbuf_data.model = transform.current_transform;
				object_cbuf_data.transposed_inverse_model = XMMatrixTranspose(XMMatrixInverse(nullptr, object_cbuf_data.model));
				object_cbuffer->Update(context, object_cbuf_data);

				if (material.albedo_texture != INVALID_TEXTURE_HANDLE)
				{
					auto view = g_TextureManager.GetTextureView(material.albedo_texture);
					context->PSSetShaderResources(TEXTURE_SLOT_DIFFUSE, 1, &view);
				}
				mesh.Draw(context);
			}
		}
		gbuffer_pass.End(context);
	}
	void Renderer::PassDecals()
	{
		if (reg.size<Decal>() == 0) return;
		ID3D11DeviceContext* context = gfx->Context();
		AdriaGfxProfileCondScope(context, "Decal Pass", profiling_enabled); 
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Decal Pass");

		struct DecalCBuffer
		{
			int32 decal_type;
			bool32 modify_gbuffer_normals;
		};

		static GfxConstantBuffer<DecalCBuffer> decal_cbuffer(gfx->Device());
		static DecalCBuffer decal_cbuf_data{};
		decal_cbuffer.Bind(context, GfxShaderStage::PS, 11);

		decal_pass.Begin(context);
		{
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			BindVertexBuffer(context, cube_vb.get());
			BindIndexBuffer(context, cube_ib.get());
			auto decal_view = reg.view<Decal>();

			context->RSSetState(cull_none.Get());
			for (auto e : decal_view)
			{
				Decal decal = decal_view.get(e);
				decal.modify_gbuffer_normals ? ShaderManager::GetShaderProgram(ShaderProgram::Decals_ModifyNormals)->Bind(context) : ShaderManager::GetShaderProgram(ShaderProgram::Decals)->Bind(context); //refactor this 

				decal_cbuf_data.decal_type = static_cast<int32>(decal.decal_type);
				decal_cbuffer.Update(context, decal_cbuf_data);

				object_cbuf_data.model = decal.decal_model_matrix;
				object_cbuf_data.transposed_inverse_model = XMMatrixTranspose(XMMatrixInverse(nullptr, object_cbuf_data.model));
				object_cbuffer->Update(context, object_cbuf_data);

				ID3D11ShaderResourceView* srvs[] = { g_TextureManager.GetTextureView(decal.albedo_decal_texture), 
													 g_TextureManager.GetTextureView(decal.normal_decal_texture),
													 depth_target->SRV() };
				context->PSSetShaderResources(0, ARRAYSIZE(srvs), srvs);
				context->DrawIndexed(cube_ib->GetCount(), 0, 0);
			}
			context->RSSetState(nullptr);
		}
		decal_pass.End(context);
	}

	void Renderer::PassSSAO()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.ambient_occlusion == AmbientOcclusion::SSAO);
		AdriaGfxProfileCondScope(context, "SSAO Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"SSAO Pass");

		static ID3D11ShaderResourceView* const srv_null[3] = { nullptr, nullptr, nullptr };
		context->PSSetShaderResources(7, 1, srv_null);
		{
			for (uint32 i = 0; i < ssao_kernel.size(); ++i)
				postprocess_cbuf_data.samples[i] = ssao_kernel[i];

			postprocess_cbuf_data.noise_scale = Vector2((float)width / 8, (float)height / 8);
			postprocess_cbuf_data.ssao_power = renderer_settings.ssao_power;
			postprocess_cbuf_data.ssao_radius = renderer_settings.ssao_radius;
			postprocess_cbuffer->Update(context, postprocess_cbuf_data);
		}

		ssao_pass.Begin(context);
		{
			ID3D11ShaderResourceView* srvs[] = { gbuffer[EGBufferSlot_NormalMetallic]->SRV(), depth_target->SRV(), ssao_random_texture->SRV() };
			context->PSSetShaderResources(1, ARRAYSIZE(srvs), srvs);
			context->IASetInputLayout(nullptr);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			ShaderManager::GetShaderProgram(ShaderProgram::SSAO)->Bind(context);
			context->Draw(4, 0);
			context->PSSetShaderResources(1, ARRAYSIZE(srv_null), srv_null);
		}
		ssao_pass.End(context);
		BlurTexture(ao_texture.get());
		ID3D11ShaderResourceView* blurred_ssao = blur_texture_final->SRV();
		context->PSSetShaderResources(7, 1, &blurred_ssao);
	}
	void Renderer::PassHBAO()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.ambient_occlusion == AmbientOcclusion::HBAO);
		AdriaGfxProfileCondScope(context, "HBAO Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"HBAO Pass");

		static ID3D11ShaderResourceView* const srv_null[3] = { nullptr, nullptr, nullptr };
		context->PSSetShaderResources(7, 1, srv_null);
		{
			postprocess_cbuf_data.noise_scale = Vector2((float)width / 8, (float)height / 8);
			postprocess_cbuf_data.hbao_r2 = renderer_settings.hbao_radius * renderer_settings.hbao_radius;
			postprocess_cbuf_data.hbao_radius_to_screen = renderer_settings.hbao_radius * 0.5f * float(height) / (tanf(camera->Fov() * 0.5f) * 2.0f);
			postprocess_cbuf_data.hbao_power = renderer_settings.hbao_power;
			postprocess_cbuffer->Update(context, postprocess_cbuf_data);
		}

		hbao_pass.Begin(context);
		{
			ID3D11ShaderResourceView* srvs[] = { gbuffer[EGBufferSlot_NormalMetallic]->SRV(), depth_target->SRV(), hbao_random_texture->SRV() };
			context->PSSetShaderResources(1, ARRAYSIZE(srvs), srvs);
			context->IASetInputLayout(nullptr);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			ShaderManager::GetShaderProgram(ShaderProgram::HBAO)->Bind(context);
			context->Draw(4, 0);
			context->PSSetShaderResources(1, ARRAYSIZE(srv_null), srv_null);
		}
		hbao_pass.End(context);
		BlurTexture(ao_texture.get());

		ID3D11ShaderResourceView* blurred_ssao = blur_texture_final->SRV();
		context->PSSetShaderResources(7, 1, &blurred_ssao);
	}
	void Renderer::PassAmbient()
	{
		ID3D11DeviceContext* context = gfx->Context();
		AdriaGfxProfileCondScope(context, "Ambient Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Ambient Pass");

		ID3D11ShaderResourceView* srvs[] = { gbuffer[EGBufferSlot_NormalMetallic]->SRV(),gbuffer[EGBufferSlot_DiffuseRoughness]->SRV(), depth_target->SRV(), gbuffer[EGBufferSlot_Emissive]->SRV() };
		context->PSSetShaderResources(0, ARRAYSIZE(srvs), srvs);

		ambient_pass.Begin(context);
		if (renderer_settings.ibl)
		{
			std::vector<ID3D11ShaderResourceView*> ibl_views{ env_srv.Get(),irmap_srv.Get(), brdf_srv.Get() };
			context->PSSetShaderResources(8, static_cast<uint32>(ibl_views.size()), ibl_views.data());
		}

		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		if (!ibl_textures_generated) renderer_settings.ibl = false;
		bool has_ao = renderer_settings.ambient_occlusion != AmbientOcclusion::None;
		if (has_ao && renderer_settings.ibl) ShaderManager::GetShaderProgram(ShaderProgram::AmbientPBR_AO_IBL)->Bind(context);
		else if (has_ao && !renderer_settings.ibl) ShaderManager::GetShaderProgram(ShaderProgram::AmbientPBR_AO)->Bind(context);
		else if (!has_ao && renderer_settings.ibl) ShaderManager::GetShaderProgram(ShaderProgram::AmbientPBR_IBL)->Bind(context);
		else ShaderManager::GetShaderProgram(ShaderProgram::AmbientPBR)->Bind(context);
		context->Draw(4, 0);

		ambient_pass.End(context);

		static ID3D11ShaderResourceView* null_srv[] = { nullptr, nullptr, nullptr, nullptr };
		context->PSSetShaderResources(0, ARRAYSIZE(null_srv), null_srv);
		context->PSSetShaderResources(7, ARRAYSIZE(null_srv), null_srv);
	}
	void Renderer::PassDeferredLighting()
	{
		ID3D11DeviceContext* context = gfx->Context();
		AdriaGfxProfileCondScope(context, "Deferred Lighting Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Deferred Lighting Pass");

		context->OMSetBlendState(additive_blend.Get(), nullptr, 0xffffffff);
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
				light_cbuf_data.type = static_cast<int32>(light_data.type);
				light_cbuf_data.use_cascades = light_data.use_cascades;
				light_cbuf_data.volumetric_strength = light_data.volumetric_strength;
				light_cbuf_data.sscs = light_data.screen_space_contact_shadows;
				light_cbuf_data.sscs_thickness = light_data.sscs_thickness;
				light_cbuf_data.sscs_max_ray_distance = light_data.sscs_max_ray_distance;
				light_cbuf_data.sscs_max_depth_distance = light_data.sscs_max_depth_distance;
				
				Matrix camera_view = camera->View();
				light_cbuf_data.position = Vector4::Transform(light_cbuf_data.position, camera_view);
				light_cbuf_data.direction = Vector4::Transform(light_cbuf_data.direction, camera_view);
				light_cbuffer->Update(context, light_cbuf_data);
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
			lighting_pass.Begin(context);
			{
				ID3D11ShaderResourceView* shader_views[3] = { nullptr };
				shader_views[0] = gbuffer[EGBufferSlot_NormalMetallic]->SRV();
				shader_views[1] = gbuffer[EGBufferSlot_DiffuseRoughness]->SRV();
				shader_views[2] = depth_target->SRV();

				context->PSSetShaderResources(0, ARRAYSIZE(shader_views), shader_views);

				context->IASetInputLayout(nullptr);
				context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

				ShaderManager::GetShaderProgram(ShaderProgram::LightingPBR)->Bind(context);
				context->Draw(4, 0);
				
				static ID3D11ShaderResourceView* null_srv[] = { nullptr, nullptr, nullptr };
				context->PSSetShaderResources(0, ARRAYSIZE(null_srv), null_srv);

				if (light_data.volumetric) PassVolumetric(light_data);
			}
			lighting_pass.End(context);


		}
		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}
	void Renderer::PassDeferredTiledLighting()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.use_tiled_deferred);
		AdriaGfxProfileCondScope(context, "Deferred Tiled Lighting Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Deferred Tiled Lighting Pass");

		ID3D11ShaderResourceView* shader_views[3] = { nullptr };
		shader_views[0] = gbuffer[EGBufferSlot_NormalMetallic]->SRV();
		shader_views[1] = gbuffer[EGBufferSlot_DiffuseRoughness]->SRV();
		shader_views[2] = depth_target->SRV();
		context->CSSetShaderResources(0, ARRAYSIZE(shader_views), shader_views);
		ID3D11ShaderResourceView* lights_srv = lights->SRV();
		context->CSSetShaderResources(3, 1, &lights_srv);
		ID3D11UnorderedAccessView* texture_uav = uav_target->UAV();
		context->CSSetUnorderedAccessViews(0, 1, &texture_uav, nullptr);

		ID3D11UnorderedAccessView* debug_uav = debug_tiled_texture->UAV();
		if (renderer_settings.visualize_tiled)
		{
			context->CSSetUnorderedAccessViews(1, 1, &debug_uav, nullptr);
		}

		ShaderManager::GetShaderProgram(ShaderProgram::TiledLighting)->Bind(context);
		context->Dispatch((uint32)std::ceil(width * 1.0f / 16), (uint32)std::ceil(height * 1.0f / 16), 1);

		ID3D11ShaderResourceView* null_srv[4] = { nullptr };
		context->CSSetShaderResources(0, ARRAYSIZE(null_srv), null_srv);
		ID3D11UnorderedAccessView* null_uav = nullptr;
		context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);

		auto rtv = hdr_render_target->RTV();
		context->OMSetRenderTargets(1, &rtv, nullptr);

		if (renderer_settings.visualize_tiled)
		{
			context->CSSetUnorderedAccessViews(1, 1, &null_uav, nullptr);
			context->OMSetBlendState(alpha_blend.Get(), nullptr, 0xfffffff);
			AddTextures(uav_target.get(), debug_tiled_texture.get());
			context->OMSetBlendState(nullptr, nullptr, 0xfffffff);
		}
		else
		{
			context->OMSetBlendState(additive_blend.Get(), nullptr, 0xfffffff);
			CopyTexture(uav_target.get());
			context->OMSetBlendState(nullptr, nullptr, 0xfffffff);
		}
		
		float black[4] = { 0.0f,0.0f,0.0f,0.0f };
		context->ClearUnorderedAccessViewFloat(debug_uav, black);
		context->ClearUnorderedAccessViewFloat(texture_uav, black);

		std::vector<Light> volumetric_lights{};

		auto light_view = reg.view<Light>();
		for (auto e : light_view)
		{
			auto const& light = light_view.get(e);
			if (light.volumetric) volumetric_lights.push_back(light);
		}

		//Volumetric lighting
		if (renderer_settings.visualize_tiled || volumetric_lights.empty()) return;
		lighting_pass.Begin(context);
		context->OMSetBlendState(additive_blend.Get(), nullptr, 0xffffffff);
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
			light_cbuffer->Update(context, light_cbuf_data);

			PassVolumetric(light);
		}
		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		lighting_pass.End(context);
	}
	void Renderer::PassDeferredClusteredLighting()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.use_clustered_deferred);
		AdriaGfxProfileCondScope(context, "Deferred Clustered Lighting Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Deferred Clustered Lighting Pass");

		if (recreate_clusters)
		{
			ID3D11UnorderedAccessView* clusters_uav = clusters->UAV();
			context->CSSetUnorderedAccessViews(0, 1, &clusters_uav, nullptr);

			ShaderManager::GetShaderProgram(ShaderProgram::ClusterBuilding)->Bind(context);
			context->Dispatch(CLUSTER_SIZE_X, CLUSTER_SIZE_Y, CLUSTER_SIZE_Z);
			ShaderManager::GetShaderProgram(ShaderProgram::ClusterBuilding)->Unbind(context);

			ID3D11UnorderedAccessView* null_uav = nullptr;
			context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);

			recreate_clusters = false;
		}

		ID3D11ShaderResourceView* srvs[] = { clusters->SRV(), lights->SRV() };
		context->CSSetShaderResources(0, ARRAYSIZE(srvs), srvs);
		ID3D11UnorderedAccessView* uavs[] = { light_counter->UAV(), light_list->UAV(), light_grid->UAV() };
		context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, nullptr);

		ShaderManager::GetShaderProgram(ShaderProgram::ClusterCulling)->Bind(context);
		context->Dispatch(CLUSTER_SIZE_X / 16, CLUSTER_SIZE_Y / 16, CLUSTER_SIZE_Z / 4);
		ShaderManager::GetShaderProgram(ShaderProgram::ClusterCulling)->Unbind(context);

		ID3D11ShaderResourceView* null_srvs[2] = { nullptr };
		context->CSSetShaderResources(0, ARRAYSIZE(null_srvs), null_srvs);
		ID3D11UnorderedAccessView* null_uavs[3] = { nullptr };
		context->CSSetUnorderedAccessViews(0, ARRAYSIZE(null_uavs), null_uavs, nullptr);

		context->OMSetBlendState(additive_blend.Get(), nullptr, 0xffffffff);

		lighting_pass.Begin(context);
		{
			ID3D11ShaderResourceView* shader_views[6] = { nullptr };
			shader_views[0] = gbuffer[EGBufferSlot_NormalMetallic]->SRV();
			shader_views[1] = gbuffer[EGBufferSlot_DiffuseRoughness]->SRV();
			shader_views[2] = depth_target->SRV();
			shader_views[3] = lights->SRV();
			shader_views[4] = light_list->SRV();
			shader_views[5] = light_grid->SRV();

			context->PSSetShaderResources(0, ARRAYSIZE(shader_views), shader_views);

			context->IASetInputLayout(nullptr);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			ShaderManager::GetShaderProgram(ShaderProgram::ClusterLightingPBR)->Bind(context);
			context->Draw(4, 0);

			static ID3D11ShaderResourceView* null_srv[6] = { nullptr};
			context->PSSetShaderResources(0, ARRAYSIZE(null_srv), null_srv);

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
				light_cbuf_data.position = XMVector4Transform(light_cbuf_data.position, camera_view);
				light_cbuf_data.direction = XMVector4Transform(light_cbuf_data.direction, camera_view);
				light_cbuffer->Update(context, light_cbuf_data);

				PassVolumetric(light);
			}
		}
		lighting_pass.End(context);
		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}
	void Renderer::PassForward()
	{
		ID3D11DeviceContext* context = gfx->Context();
		AdriaGfxProfileCondScope(context, "Forward Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Forward Pass");

		forward_pass.Begin(context); 
		PassForwardCommon(false);
		PassSky();
		PassOcean();
		PassAABB();
		PassForwardCommon(true);
		forward_pass.End(context);
	}
	void Renderer::PassVoxelize()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.voxel_gi);
		AdriaGfxProfileCondScope(context, "Voxelization Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Voxelization Pass");

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
		lights->Update(_lights.data(), std::min<uint64>(_lights.size(), VOXELIZE_MAX_LIGHTS) * sizeof(LightSBuffer));

		auto voxel_view = reg.view<Mesh, Transform, Material, Deferred, AABB>();

		ShaderManager::GetShaderProgram(ShaderProgram::Voxelize)->Bind(context);
		context->RSSetState(cull_none.Get());

		D3D11_VIEWPORT vp{};
		vp.Width = VOXEL_RESOLUTION;
		vp.Height = VOXEL_RESOLUTION;
		vp.MaxDepth = 1.0f;
		context->RSSetViewports(1, &vp);


		ID3D11UnorderedAccessView* voxels_uav = voxels->UAV();
		context->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr,
			0, 1, &voxels_uav, nullptr);

		ID3D11ShaderResourceView* lights_srv = lights->SRV();
		context->PSSetShaderResources(10, 1, &lights_srv);

		for (auto e : voxel_view)
		{
			auto [mesh, transform, material, aabb] = voxel_view.get<Mesh, Transform, Material, AABB>(e);

			if (!aabb.camera_visible) continue;

			object_cbuf_data.model = transform.current_transform;
			object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
			object_cbuffer->Update(context, object_cbuf_data);

			material_cbuf_data.albedo_factor = material.albedo_factor;
			material_cbuffer->Update(context, material_cbuf_data);
			auto view = g_TextureManager.GetTextureView(material.albedo_texture);

			context->PSSetShaderResources(TEXTURE_SLOT_DIFFUSE, 1, &view);
			mesh.Draw(context);
		}

		context->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr,
			0, 0, nullptr, nullptr);

		context->RSSetState(nullptr);
		ShaderManager::GetShaderProgram(ShaderProgram::Voxelize)->Unbind(context);

		ID3D11UnorderedAccessView* uavs[] = { voxels_uav, voxel_texture->UAV() };
		context->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
		ShaderManager::GetShaderProgram(ShaderProgram::VoxelCopy)->Bind(context);
		context->Dispatch(VOXEL_RESOLUTION * VOXEL_RESOLUTION * VOXEL_RESOLUTION / 256, 1, 1);
		ShaderManager::GetShaderProgram(ShaderProgram::VoxelCopy)->Unbind(context);

		static ID3D11UnorderedAccessView* null_uavs[] = { nullptr, nullptr };
		context->CSSetUnorderedAccessViews(0, 2, null_uavs, nullptr);

		context->GenerateMips(voxel_texture->SRV());

		if (renderer_settings.voxel_second_bounce)
		{
			ID3D11UnorderedAccessView* uavs[] = { voxel_texture_second_bounce->UAV() };
			context->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

			ID3D11ShaderResourceView* srvs[] = { voxels->SRV(), voxel_texture->SRV() };
			context->CSSetShaderResources(0, 2, srvs);
			ShaderManager::GetShaderProgram(ShaderProgram::VoxelSecondBounce)->Bind(context);
			context->Dispatch(VOXEL_RESOLUTION / 8, VOXEL_RESOLUTION / 8, VOXEL_RESOLUTION / 8);
			ShaderManager::GetShaderProgram(ShaderProgram::VoxelSecondBounce)->Unbind(context);

			static ID3D11ShaderResourceView* null_srvs[] = { nullptr, nullptr };
			context->CSSetShaderResources(0, 2, null_srvs);

			static ID3D11UnorderedAccessView* null_uavs[] = { nullptr};
			context->CSSetUnorderedAccessViews(0, 1, null_uavs, nullptr);

			context->GenerateMips(voxel_texture_second_bounce->SRV());
		}
	}
	void Renderer::PassVoxelizeDebug()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.voxel_gi && renderer_settings.voxel_debug);
		AdriaGfxProfileCondScope(context, "Voxelization Debug Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Voxelization Debug Pass");

		voxel_debug_pass.Begin(context);
		{
			ID3D11ShaderResourceView* voxel_srv = voxel_texture->SRV();
			context->VSSetShaderResources(9, 1, &voxel_srv);

			ShaderManager::GetShaderProgram(ShaderProgram::VoxelizeDebug)->Bind(context);
			context->IASetInputLayout(nullptr);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
			context->Draw(VOXEL_RESOLUTION * VOXEL_RESOLUTION * VOXEL_RESOLUTION, 0);
			ShaderManager::GetShaderProgram(ShaderProgram::VoxelizeDebug)->Unbind(context);

			static ID3D11ShaderResourceView* null_srv = nullptr;
			context->VSSetShaderResources(9, 1, &null_srv);
		}
		voxel_debug_pass.End(context);
	}
	void Renderer::PassVoxelGI()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.voxel_gi);
		AdriaGfxProfileCondScope(context, "Voxel GI Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Voxel GI Pass");

		context->OMSetBlendState(additive_blend.Get(), nullptr, 0xffffffff);
		lighting_pass.Begin(context);
		{
			ID3D11ShaderResourceView* shader_views[3] = { nullptr };
			shader_views[0] = gbuffer[EGBufferSlot_NormalMetallic]->SRV();
			shader_views[1] = depth_target->SRV();
			shader_views[2] = renderer_settings.voxel_second_bounce ? voxel_texture_second_bounce->SRV() : voxel_texture->SRV();
			context->PSSetShaderResources(0, ARRAYSIZE(shader_views), shader_views);

			context->IASetInputLayout(nullptr);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			ShaderManager::GetShaderProgram(ShaderProgram::VoxelGI)->Bind(context);
			context->Draw(4, 0);
			ShaderManager::GetShaderProgram(ShaderProgram::VoxelGI)->Unbind(context);
			static ID3D11ShaderResourceView* null_srv[] = { nullptr, nullptr, nullptr };
			context->PSSetShaderResources(0, ARRAYSIZE(null_srv), null_srv);
		}
		lighting_pass.End(context);
		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}
	void Renderer::PassPostprocessing()
	{
		ID3D11DeviceContext* context = gfx->Context();
		AdriaGfxProfileCondScope(context, "Postprocessing Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Postprocessing Pass");

		PassVelocityBuffer();

		auto lights = reg.view<Light>();
		postprocess_passes[postprocess_index].Begin(context); 
		CopyTexture(hdr_render_target.get()); //replace this with CopyResource!
		context->OMSetBlendState(additive_blend.Get(), nullptr, 0xffffffff);
		for (entity light : lights)
		{
			auto const& light_data = lights.get(light);
			if (!light_data.active || !light_data.lens_flare) continue;
			PassLensFlare(light_data);
		}
		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);

		postprocess_passes[postprocess_index].End(context); 
		postprocess_index = !postprocess_index;

		if (renderer_settings.dof)
		{
			postprocess_passes[postprocess_index].Begin(context);
			PassDepthOfField();
			postprocess_passes[postprocess_index].End(context);

			postprocess_index = !postprocess_index;
		}

		if (renderer_settings.clouds)
		{
			postprocess_passes[postprocess_index].Begin(context);
			PassVolumetricClouds();
			postprocess_passes[postprocess_index].End(context);

			postprocess_index = !postprocess_index;
		}

		if (renderer_settings.fog)
		{
			postprocess_passes[postprocess_index].Begin(context);
			PassFog();
			postprocess_passes[postprocess_index].End(context);

			postprocess_index = !postprocess_index;
		}

		if (renderer_settings.ssr)
		{
			postprocess_passes[postprocess_index].Begin(context);
			PassSSR();
			postprocess_passes[postprocess_index].End(context);

			postprocess_index = !postprocess_index;
		}

		if (renderer_settings.bloom)
		{
			PassBloom();
			postprocess_index = !postprocess_index;
		}

		if (renderer_settings.motion_blur)
		{
			postprocess_passes[postprocess_index].Begin(context);
			PassMotionBlur();
			postprocess_passes[postprocess_index].End(context);

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
					postprocess_passes[!postprocess_index].Begin(context);
					PassGodRays(light_data);
					postprocess_passes[!postprocess_index].End(context);
				}
				else
				{
					DrawSun(light);
					postprocess_passes[!postprocess_index].Begin(context);
					context->OMSetBlendState(additive_blend.Get(), nullptr, 0xffffffff);
					CopyTexture(sun_target.get());
					context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
					postprocess_passes[!postprocess_index].End(context);
				}
			}
		}

		if (renderer_settings.anti_aliasing & EAntiAliasing_TAA)
		{
			postprocess_passes[postprocess_index].Begin(context);
			PassTAA();
			postprocess_passes[postprocess_index].End(context);
			postprocess_index = !postprocess_index;

			auto rtv = prev_hdr_render_target->RTV();
			context->OMSetRenderTargets(1, &rtv, nullptr);
			CopyTexture(postprocess_textures[!postprocess_index].get());
			context->OMSetRenderTargets(0, nullptr, nullptr);
		}
	}

	void Renderer::PassShadowMapDirectional(Light const& light)
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(light.type == LightType::Directional);
		AdriaGfxProfileCondScope(context, "Directional Shadow Map Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Directional Shadow Map Pass");

		auto const& [V, P] = scene_bounding_sphere ? LightViewProjection_Directional(light, *scene_bounding_sphere, light_bounding_box)
			: LightViewProjection_Directional(light, *camera, light_bounding_box);
		shadow_cbuf_data.lightview = V;
		shadow_cbuf_data.lightviewprojection = V * P;
		shadow_cbuf_data.shadow_map_size = SHADOW_MAP_SIZE;
		shadow_cbuf_data.softness = renderer_settings.shadow_softness;
		shadow_cbuf_data.shadow_matrices[0] = camera->View().Invert() * shadow_cbuf_data.lightviewprojection;
		shadow_cbuffer->Update(context, shadow_cbuf_data);

		ID3D11ShaderResourceView* null_srv[1] = { nullptr };
		context->PSSetShaderResources(TEXTURE_SLOT_SHADOW, 1, null_srv);
		shadow_map_pass.Begin(context);
		{
			context->RSSetState(shadow_depth_bias.Get());
			LightFrustumCulling(LightType::Directional);
			PassShadowMapCommon();
			context->RSSetState(nullptr);
		}
		shadow_map_pass.End(context);
		ID3D11ShaderResourceView* shadow_depth_srv[1] = { shadow_depth_map->SRV() };
		context->PSSetShaderResources(TEXTURE_SLOT_SHADOW, 1, shadow_depth_srv);
	}
	void Renderer::PassShadowMapSpot(Light const& light)
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(light.type == LightType::Spot);
		AdriaGfxProfileCondScope(context, "Spot Shadow Map Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Spot Shadow Map Pass");

		auto const& [V, P] = LightViewProjection_Spot(light, light_bounding_frustum);
		shadow_cbuf_data.lightview = V;
		shadow_cbuf_data.lightviewprojection = V * P;
		shadow_cbuf_data.shadow_map_size = SHADOW_MAP_SIZE;
		shadow_cbuf_data.softness = renderer_settings.shadow_softness;
		shadow_cbuf_data.shadow_matrices[0] = camera->View().Invert() * shadow_cbuf_data.lightviewprojection;
		shadow_cbuffer->Update(context, shadow_cbuf_data);

		ID3D11ShaderResourceView* null_srv[1] = { nullptr };
		context->PSSetShaderResources(TEXTURE_SLOT_SHADOW, 1, null_srv);
		shadow_map_pass.Begin(context);
		{
			context->RSSetState(shadow_depth_bias.Get());
			LightFrustumCulling(LightType::Spot);
			PassShadowMapCommon();
			context->RSSetState(nullptr);
		}
		shadow_map_pass.End(context);
		ID3D11ShaderResourceView* shadow_depth_srv[1] = { shadow_depth_map->SRV() };
		context->PSSetShaderResources(TEXTURE_SLOT_SHADOW, 1, shadow_depth_srv);
	}
	void Renderer::PassShadowMapPoint(Light const& light)
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(light.type == LightType::Point);
		AdriaGfxProfileCondScope(context, "Point Shadow Map Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Point Shadow Map Pass");

		for (uint32 i = 0; i < shadow_cubemap_pass.size(); ++i)
		{
			auto const& [V, P] = LightViewProjection_Point(light, i, light_bounding_frustum);
			shadow_cbuf_data.lightviewprojection = V * P;
			shadow_cbuf_data.lightview = V;
			shadow_cbuffer->Update(context, shadow_cbuf_data);

			ID3D11ShaderResourceView* null_srv[1] = { nullptr };
			context->PSSetShaderResources(TEXTURE_SLOT_SHADOWCUBE, 1, null_srv);

			shadow_cubemap_pass[i].Begin(context);
			{
				context->RSSetState(shadow_depth_bias.Get());
				LightFrustumCulling(LightType::Point);
				PassShadowMapCommon();
				context->RSSetState(nullptr);
			}
			shadow_cubemap_pass[i].End(context);
		}

		ID3D11ShaderResourceView* srv[] = { shadow_depth_cubemap->SRV() };
		context->PSSetShaderResources(TEXTURE_SLOT_SHADOWCUBE, 1, srv);
	}
	void Renderer::PassShadowMapCascades(Light const& light)
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(light.type == LightType::Directional);
		AdriaGfxProfileCondScope(context, "Cascades Shadow Map Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Cascades Shadow Map Pass");

		std::array<float, CASCADE_COUNT> split_distances;
		std::array<Matrix, CASCADE_COUNT> proj_matrices = RecalculateProjectionMatrices(*camera, renderer_settings.split_lambda, split_distances);
		std::array<Matrix, CASCADE_COUNT> light_view_projections{};

		ID3D11ShaderResourceView* null_srv[] = { nullptr };
		context->PSSetShaderResources(TEXTURE_SLOT_SHADOWARRAY, 1, null_srv);
		context->RSSetState(shadow_depth_bias.Get());
		
		for (uint32 i = 0; i < CASCADE_COUNT; ++i)
		{
			auto const& [V, P] = LightViewProjection_Cascades(light, *camera, proj_matrices[i], light_bounding_box);
			light_view_projections[i] = V * P;
			shadow_cbuf_data.lightview = V;
			shadow_cbuf_data.lightviewprojection = light_view_projections[i];
			shadow_cbuffer->Update(context, shadow_cbuf_data);

			cascade_shadow_pass[i].Begin(context);
			{
				LightFrustumCulling(LightType::Directional);
				PassShadowMapCommon();
			}
			cascade_shadow_pass[i].End(context);
		}

		context->RSSetState(nullptr);
		ID3D11ShaderResourceView* srv[] = { shadow_cascade_maps->SRV() };
		context->PSSetShaderResources(TEXTURE_SLOT_SHADOWARRAY, 1, srv);

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
		shadow_cbuf_data.visualize = static_cast<int>(false);
		shadow_cbuffer->Update(context, shadow_cbuf_data);
	}
	void Renderer::PassShadowMapCommon()
	{
		ID3D11DeviceContext* context = gfx->Context();
		auto shadow_view = reg.view<Mesh, Transform, AABB>();
		if (!renderer_settings.shadow_transparent)
		{
			ShaderManager::GetShaderProgram(ShaderProgram::DepthMap)->Bind(context);
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
					object_cbuffer->Update(context, object_cbuf_data);
					mesh.Draw(context);
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

			ShaderManager::GetShaderProgram(ShaderProgram::DepthMap)->Bind(context);
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
				object_cbuffer->Update(context, object_cbuf_data);
				mesh.Draw(context);
			}

			ShaderManager::GetShaderProgram(ShaderProgram::DepthMap_Transparent)->Bind(context);
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
				object_cbuffer->Update(context, object_cbuf_data);

				auto view = g_TextureManager.GetTextureView(material->albedo_texture);
				context->PSSetShaderResources(TEXTURE_SLOT_DIFFUSE, 1, &view);

				mesh.Draw(context);
			}
		}
	}

	void Renderer::PassVolumetric(Light const& light)
	{
		ID3D11DeviceContext* context = gfx->Context();
		if (light.type == LightType::Directional && !light.casts_shadows)
		{
			ADRIA_LOG(WARNING, "Volumetric Directional Light that does not cast shadows does not make sense!");
			return;
		}
		ADRIA_ASSERT(light.volumetric);
		AdriaGfxProfileCondScope(context, "Volumetric Lighting Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Volumetric Lighting Pass");

		ID3D11ShaderResourceView* srv[] = { depth_target->SRV() };
		context->PSSetShaderResources(2, 1, srv);

		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		switch (light.type)
		{
		case LightType::Directional:
			if (light.use_cascades) ShaderManager::GetShaderProgram(ShaderProgram::Volumetric_DirectionalCascades)->Bind(context);
			else ShaderManager::GetShaderProgram(ShaderProgram::Volumetric_Directional)->Bind(context);
			break;
		case LightType::Spot:
			ShaderManager::GetShaderProgram(ShaderProgram::Volumetric_Spot)->Bind(context);
			break;
		case LightType::Point:
			ShaderManager::GetShaderProgram(ShaderProgram::Volumetric_Point)->Bind(context);
			break;
		default:
			ADRIA_ASSERT(false && "Invalid Light Type!");
		}
		context->Draw(4, 0);

		static ID3D11ShaderResourceView* null_srv = nullptr;
		context->PSSetShaderResources(2, 1, &null_srv);
	}
	void Renderer::PassSky()
	{
		ID3D11DeviceContext* context = gfx->Context();
		AdriaGfxProfileCondScope(context, "Sky Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Sky Pass");

		object_cbuf_data.model = Matrix::CreateTranslation(camera->Position());
		object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
		object_cbuffer->Update(context, object_cbuf_data);

		context->RSSetState(cull_none.Get());
		context->OMSetDepthStencilState(leq_depth.Get(), 0);

		auto skybox_view = reg.view<Skybox>();
		switch (renderer_settings.sky_type)
		{
		case SkyType::Skybox:
			ShaderManager::GetShaderProgram(ShaderProgram::Skybox)->Bind(context);
			for (auto e : skybox_view)
			{
				auto const& skybox = skybox_view.get(e);
				if (!skybox.active) continue;
				auto view = g_TextureManager.GetTextureView(skybox.cubemap_texture);
				context->PSSetShaderResources(TEXTURE_SLOT_CUBEMAP, 1, &view);
				break;
			}
			break;
		case SkyType::UniformColor:
			ShaderManager::GetShaderProgram(ShaderProgram::UniformColorSky)->Bind(context);
			break;
		case SkyType::HosekWilkie:
			ShaderManager::GetShaderProgram(ShaderProgram::HosekWilkieSky)->Bind(context);
			break;
		default:
			ADRIA_ASSERT(false);
		}

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		BindVertexBuffer(context, cube_vb.get());
		BindIndexBuffer(context, cube_ib.get());
		context->DrawIndexed(cube_ib->GetCount(), 0, 0);
		context->OMSetDepthStencilState(nullptr, 0);
		context->RSSetState(nullptr);
	}
	void Renderer::PassOcean()
	{
		if (reg.size<Ocean>() == 0) return;

		ID3D11DeviceContext* context = gfx->Context();
		AdriaGfxProfileCondScope(context, "Ocean Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Ocean Pass");

		if (renderer_settings.ocean_wireframe) context->RSSetState(wireframe.Get());
		context->OMSetBlendState(alpha_blend.Get(), nullptr, 0xffffffff);

		auto skyboxes = reg.view<Skybox>();
		ID3D11ShaderResourceView* skybox_srv = nullptr;
		for (auto skybox : skyboxes)
		{
			auto const& _skybox = skyboxes.get(skybox);
			skybox_srv = g_TextureManager.GetTextureView(_skybox.cubemap_texture);
			if (skybox_srv) break;
		}

		ID3D11ShaderResourceView* displacement_map_srv = ping_pong_spectrum_textures[!pong_spectrum]->SRV();
		renderer_settings.ocean_tesselation ? context->DSSetShaderResources(0, 1, &displacement_map_srv) :
							context->VSSetShaderResources(0, 1, &displacement_map_srv);

		ID3D11ShaderResourceView* srvs[] = { ocean_normal_map->SRV(), skybox_srv ,
		 g_TextureManager.GetTextureView(foam_handle) };
		context->PSSetShaderResources(0, ARRAYSIZE(srvs), srvs);

		renderer_settings.ocean_tesselation ? ShaderManager::GetShaderProgram(ShaderProgram::OceanLOD)->Bind(context)
						  : ShaderManager::GetShaderProgram(ShaderProgram::Ocean)->Bind(context);

		auto ocean_chunk_view = reg.view<Mesh, Material, Transform, AABB, Ocean>();
		for (auto ocean_chunk : ocean_chunk_view)
		{
			auto [mesh, material, transform, aabb] = ocean_chunk_view.get<const Mesh, const Material, const Transform, const AABB>(ocean_chunk);

			if (aabb.camera_visible)
			{
				object_cbuf_data.model = transform.current_transform;
				object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
				object_cbuffer->Update(context, object_cbuf_data);
				
				material_cbuf_data.diffuse = material.diffuse;
				material_cbuffer->Update(context, material_cbuf_data);

				renderer_settings.ocean_tesselation ? mesh.Draw(context, D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST) : mesh.Draw(context);
			}
		}

		renderer_settings.ocean_tesselation ? ShaderManager::GetShaderProgram(ShaderProgram::OceanLOD)->Unbind(context) : ShaderManager::GetShaderProgram(ShaderProgram::Ocean)->Unbind(context);

		static ID3D11ShaderResourceView* null_srv = nullptr;
		renderer_settings.ocean_tesselation ? context->DSSetShaderResources(0, 1, &null_srv) :
							context->VSSetShaderResources(0, 1, &null_srv);
		context->PSSetShaderResources(0, 1, &null_srv);
		context->PSSetShaderResources(1, 1, &null_srv);
		context->PSSetShaderResources(2, 1, &null_srv);

		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		if (renderer_settings.ocean_wireframe) context->RSSetState(nullptr);
	}
	void Renderer::PassParticles()
	{
		ID3D11DeviceContext* context = gfx->Context();
		if (reg.size<Emitter>() == 0) return;
		AdriaGfxProfileCondScope(context, "Particles Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Particles Pass");

		particle_pass.Begin(context);
		context->OMSetBlendState(alpha_blend.Get(), nullptr, 0xffffffff);

		auto emitters = reg.view<Emitter>();
		for (auto emitter : emitters)
		{
			Emitter const& emitter_params = emitters.get(emitter);
			particle_renderer.Render(emitter_params, depth_target->SRV(), g_TextureManager.GetTextureView(emitter_params.particle_texture));
		}

		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		particle_pass.End(context);
	}

	void Renderer::PassAABB()
	{
		ID3D11DeviceContext* context = gfx->Context();
		auto aabb_view = reg.view<AABB>();
		for (auto e : aabb_view)
		{
			auto& aabb = aabb_view.get(e);
			if (aabb.draw_aabb)
			{
				object_cbuf_data.model = Matrix::Identity;
				object_cbuf_data.transposed_inverse_model = Matrix::Identity;
				object_cbuffer->Update(context, object_cbuf_data);

				material_cbuf_data.diffuse = Vector3(1, 0, 0);
				material_cbuffer->Update(context, material_cbuf_data);

				context->RSSetState(wireframe.Get());
				context->OMSetDepthStencilState(no_depth_test.Get(), 0);
				ShaderManager::GetShaderProgram(ShaderProgram::Solid)->Bind(context);
				context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
				BindVertexBuffer(context, aabb.aabb_vb.get());
				BindIndexBuffer(context, aabb_wireframe_ib.get());
				context->DrawIndexed(aabb_wireframe_ib->GetCount(), 0, 0);
				context->RSSetState(nullptr);
				context->OMSetDepthStencilState(nullptr, 0);

				aabb.draw_aabb = false;
				break;
			}
		}
	}

	void Renderer::PassForwardCommon(bool transparent)
	{
		ID3D11DeviceContext* context = gfx->Context();
		auto forward_view = reg.view<Mesh, Transform, AABB, Material, Forward>();

		if (transparent) context->OMSetBlendState(alpha_blend.Get(), nullptr, 0xffffffff);

		for (auto e : forward_view)
		{
			auto [forward, aabb] = forward_view.get<Forward const, AABB const>(e);

			if (!(aabb.camera_visible && forward.transparent == transparent)) continue;
			
			auto [transform, mesh, material] = forward_view.get<Transform, Mesh, Material>(e);

			ShaderManager::GetShaderProgram(material.shader)->Bind(context);

			object_cbuf_data.model = transform.current_transform;
			object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
			object_cbuffer->Update(context, object_cbuf_data);
				
			material_cbuf_data.diffuse = material.diffuse;
			material_cbuf_data.albedo_factor = material.albedo_factor;

			material_cbuffer->Update(context, material_cbuf_data);

			if (material.albedo_texture != INVALID_TEXTURE_HANDLE)
			{
				auto view = g_TextureManager.GetTextureView(material.albedo_texture);

				context->PSSetShaderResources(TEXTURE_SLOT_DIFFUSE, 1, &view);
			}

			auto const* states = reg.get_if<RenderState>(e);
				
			if (states) ResolveCustomRenderState(*states, false);
			mesh.Draw(context);
			if (states) ResolveCustomRenderState(*states, true);
		}

		if (transparent) context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}

	void Renderer::PassLensFlare(Light const& light)
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(light.lens_flare);
		AdriaGfxProfileCondScope(context, "Lens Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Lens Flare Pass");

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
		light_cbuffer->Update(context, light_cbuf_data);

		{
			ID3D11ShaderResourceView* depth_srv_array[1] = { depth_target->SRV() };
			context->GSSetShaderResources(7, ARRAYSIZE(depth_srv_array), depth_srv_array);
			context->GSSetShaderResources(0, static_cast<uint32>(lens_flare_textures.size()), lens_flare_textures.data());
			context->PSSetShaderResources(0, static_cast<uint32>(lens_flare_textures.size()), lens_flare_textures.data());

			ShaderManager::GetShaderProgram(ShaderProgram::LensFlare)->Bind(context);
			context->IASetInputLayout(nullptr);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
			context->Draw(7, 0);
			ShaderManager::GetShaderProgram(ShaderProgram::LensFlare)->Unbind(context);

			static ID3D11ShaderResourceView* const srv_null[1] = { nullptr };
			static std::vector<ID3D11ShaderResourceView*> const lens_null_array(lens_flare_textures.size(), nullptr);
			context->GSSetShaderResources(7, ARRAYSIZE(srv_null), srv_null);
			context->GSSetShaderResources(0, static_cast<uint32>(lens_null_array.size()), lens_null_array.data());
			context->PSSetShaderResources(0, static_cast<uint32>(lens_null_array.size()), lens_null_array.data());
		}
	}
	void Renderer::PassVolumetricClouds()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.clouds);
		AdriaGfxProfileCondScope(context, "Volumetric Clouds Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Volumetric Clouds Pass");

		ID3D11ShaderResourceView* const srv_array[] = { clouds_textures[0], clouds_textures[1], clouds_textures[2], depth_target->SRV()};
		context->PSSetShaderResources(0, ARRAYSIZE(srv_array), srv_array);
		context->IASetInputLayout(nullptr);

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		ShaderManager::GetShaderProgram(ShaderProgram::Volumetric_Clouds)->Bind(context);
		context->Draw(4, 0);

		static ID3D11ShaderResourceView* const srv_null[] = { nullptr, nullptr, nullptr, nullptr };
		context->PSSetShaderResources(0, ARRAYSIZE(srv_null), srv_null);

		postprocess_passes[postprocess_index].End(context);
		postprocess_index = !postprocess_index;
		postprocess_passes[postprocess_index].Begin(context);

		BlurTexture(postprocess_textures[!postprocess_index].get());
		context->OMSetBlendState(alpha_blend.Get(), nullptr, 0xfffffff);
		CopyTexture(blur_texture_final.get());
		context->OMSetBlendState(nullptr, nullptr, 0xfffffff);
	}
	void Renderer::PassSSR()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.ssr);
		AdriaGfxProfileCondScope(context, "SSR Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"SSR Pass");

		postprocess_cbuf_data.ssr_ray_hit_threshold = renderer_settings.ssr_ray_hit_threshold;
		postprocess_cbuf_data.ssr_ray_step = renderer_settings.ssr_ray_step;
		postprocess_cbuffer->Update(context, postprocess_cbuf_data);

		ID3D11ShaderResourceView* srv_array[] = { gbuffer[EGBufferSlot_NormalMetallic]->SRV(), postprocess_textures[!postprocess_index]->SRV(), depth_target->SRV() };
		context->PSSetShaderResources(0, ARRAYSIZE(srv_array), srv_array);
		context->IASetInputLayout(nullptr);

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		ShaderManager::GetShaderProgram(ShaderProgram::SSR)->Bind(context);
		context->Draw(4, 0);
		static ID3D11ShaderResourceView* const srv_null[] = { nullptr, nullptr, nullptr };
		context->PSSetShaderResources(0, ARRAYSIZE(srv_null), srv_null);
	}
	void Renderer::PassGodRays(Light const& light)
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(light.god_rays);
		AdriaGfxProfileCondScope(context, "God Rays Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"God Rays Pass");

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

			static float const max_light_dist = 1.3f;

			float f_max_dist = std::max(abs(light_cbuf_data.screenspace_position.x), abs(light_cbuf_data.screenspace_position.y));
			if (f_max_dist >= 1.0f) light_cbuf_data.color = Vector4::Transform(light_cbuf_data.color, Matrix::CreateScale((max_light_dist - f_max_dist), (max_light_dist - f_max_dist), (max_light_dist - f_max_dist)));

			light_cbuffer->Update(context, light_cbuf_data);
		}

		context->OMSetBlendState(additive_blend.Get(), nullptr, 0xffffffff);
		{
			ID3D11ShaderResourceView* srv_array[1] = { sun_target->SRV() };
			static ID3D11ShaderResourceView* const srv_null[1] = { nullptr };

			context->PSSetShaderResources(0, ARRAYSIZE(srv_array), srv_array);
			context->IASetInputLayout(nullptr);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			ShaderManager::GetShaderProgram(ShaderProgram::GodRays)->Bind(context);
			context->Draw(4, 0);
			context->PSSetShaderResources(0, ARRAYSIZE(srv_null), srv_null);
		}
		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}
	void Renderer::PassDepthOfField()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.dof);
		AdriaGfxProfileCondScope(context, "Depth of Field Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Depth of Field Pass");

		if (renderer_settings.bokeh)
		{
			ShaderManager::GetShaderProgram(ShaderProgram::BokehGenerate)->Bind(context);
			ID3D11ShaderResourceView* srv_array[2] = { postprocess_textures[!postprocess_index]->SRV(), depth_target->SRV() };
			uint32 initial_count = 0;
			ID3D11UnorderedAccessView* bokeh_uav = bokeh_buffer->UAV();
			context->CSSetUnorderedAccessViews(0, 1, &bokeh_uav, &initial_count);
			context->CSSetShaderResources(0, 2, srv_array);
			context->Dispatch((uint32)std::ceil(width / 32.0f), (uint32)std::ceil(height / 32.0f), 1);

			static ID3D11UnorderedAccessView* null_uav = nullptr;
			context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);
			static ID3D11ShaderResourceView* null_srvs[2] = { nullptr, nullptr };
			context->CSSetShaderResources(0, 2, null_srvs);
			ShaderManager::GetShaderProgram(ShaderProgram::BokehGenerate)->Unbind(context);
		}

		BlurTexture(hdr_render_target.get());

		ID3D11ShaderResourceView* srv_array[3] = { postprocess_textures[!postprocess_index]->SRV(), blur_texture_final->SRV(), depth_target->SRV()};
		static ID3D11ShaderResourceView* const srv_null[3] = { nullptr, nullptr, nullptr };

		postprocess_cbuf_data.dof_params = XMVectorSet(renderer_settings.dof_near_blur, renderer_settings.dof_near, renderer_settings.dof_far, renderer_settings.dof_far_blur);
		postprocess_cbuffer->Update(context, postprocess_cbuf_data);

		context->PSSetShaderResources(0, ARRAYSIZE(srv_array), srv_array);
		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		ShaderManager::GetShaderProgram(ShaderProgram::DOF)->Bind(context);
		context->Draw(4, 0);
		context->PSSetShaderResources(0, ARRAYSIZE(srv_null), srv_null);

		if (renderer_settings.bokeh)
		{
			context->CopyStructureCount(bokeh_indirect_draw_buffer->GetNative(), 0, bokeh_buffer->UAV());
			ID3D11ShaderResourceView* bokeh = nullptr;
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

			ID3D11ShaderResourceView* bokeh_srv = bokeh_buffer->SRV();
			context->VSSetShaderResources(0, 1, &bokeh_srv);
			context->PSSetShaderResources(0, 1, &bokeh);

			ID3D11Buffer* vertexBuffers[1] = { nullptr };
			uint32 strides[1] = { 0 };
			uint32 offsets[1] = { 0 };
			context->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
			context->IASetInputLayout(nullptr);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
			context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

			ShaderManager::GetShaderProgram(ShaderProgram::BokehDraw)->Bind(context);
			context->OMSetBlendState(additive_blend.Get(), nullptr, 0xfffffff);
			context->DrawInstancedIndirect(bokeh_indirect_draw_buffer->GetNative(), 0);
			context->OMSetBlendState(nullptr, nullptr, 0xfffffff);
			static ID3D11ShaderResourceView* null_srv = nullptr;
			context->VSSetShaderResources(0, 1, &null_srv);
			context->PSSetShaderResources(0, 1, &null_srv);
			ShaderManager::GetShaderProgram(ShaderProgram::BokehDraw)->Unbind(context);
		}
	}
	void Renderer::PassBloom()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.bloom);
		AdriaGfxProfileCondScope(context, "Bloom Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Bloom Pass");

		static ID3D11UnorderedAccessView* const null_uav[1] = { nullptr };
		static ID3D11ShaderResourceView*  const null_srv[2] = { nullptr };

		ID3D11UnorderedAccessView* const uav[1] = { bloom_extract_texture->UAV() };
		ID3D11ShaderResourceView* const srv[1] = { postprocess_textures[!postprocess_index]->SRV() };
		context->CSSetShaderResources(0, 1, srv); 

		context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uav), uav, nullptr);
		ShaderManager::GetShaderProgram(ShaderProgram::BloomExtract)->Bind(context);
		context->Dispatch((uint32)std::ceil(width / 32.0f), (uint32)std::ceil(height / 32.0f), 1);
		context->CSSetShaderResources(0, 1, null_srv);
		context->CSSetUnorderedAccessViews(0, 1, null_uav, nullptr);
		context->GenerateMips(bloom_extract_texture->SRV());

		ID3D11UnorderedAccessView* const uav2[1] = { postprocess_textures[postprocess_index]->UAV() };
		ID3D11ShaderResourceView* const srv2[2] = { postprocess_textures[!postprocess_index]->SRV(), bloom_extract_texture->SRV() };
		context->CSSetShaderResources(0, 2, srv2);
		context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uav2), uav2, nullptr);
		ShaderManager::GetShaderProgram(ShaderProgram::BloomCombine)->Bind(context);
		context->Dispatch((uint32)std::ceil(width / 32.0f), (uint32)std::ceil(height / 32.0f), 1);

		context->CSSetShaderResources(0, 2, null_srv);
		context->CSSetUnorderedAccessViews(0, 1, null_uav, nullptr);
	}
	void Renderer::PassVelocityBuffer()
	{
		ID3D11DeviceContext* context = gfx->Context();
		if (!renderer_settings.motion_blur && !(renderer_settings.anti_aliasing & EAntiAliasing_TAA)) return;
		AdriaGfxProfileCondScope(context, "Velocity Buffer Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Velocity Buffer Pass");

		postprocess_cbuf_data.velocity_buffer_scale = renderer_settings.velocity_buffer_scale;
		postprocess_cbuffer->Update(context, postprocess_cbuf_data);

		velocity_buffer_pass.Begin(context);
		{
			ID3D11ShaderResourceView* const srv_array[1] = { depth_target->SRV() };
			static ID3D11ShaderResourceView* const srv_null[1] = { nullptr };

			context->PSSetShaderResources(0, ARRAYSIZE(srv_array), srv_array);
			context->IASetInputLayout(nullptr);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			ShaderManager::GetShaderProgram(ShaderProgram::VelocityBuffer)->Bind(context);
			context->Draw(4, 0);
			context->PSSetShaderResources(0, ARRAYSIZE(srv_null), srv_null);
		}
		velocity_buffer_pass.End(context);
	}

	void Renderer::PassMotionBlur()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.motion_blur);
		AdriaGfxProfileCondScope(context, "Motion Blur Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Motion Blur Pass");

		ID3D11ShaderResourceView* const srv_array[2] = { postprocess_textures[!postprocess_index]->SRV(), velocity_buffer->SRV() };
		static ID3D11ShaderResourceView* const srv_null[2] = { nullptr, nullptr };

		context->PSSetShaderResources(0, ARRAYSIZE(srv_array), srv_array);
		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		ShaderManager::GetShaderProgram(ShaderProgram::MotionBlur)->Bind(context);
		context->Draw(4, 0);
		context->PSSetShaderResources(0, ARRAYSIZE(srv_null), srv_null);
	}
	void Renderer::PassFog()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.fog);
		AdriaGfxProfileCondScope(context, "Fog Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Fog Pass");

		postprocess_cbuf_data.fog_falloff = renderer_settings.fog_falloff;
		postprocess_cbuf_data.fog_density = renderer_settings.fog_density;
		postprocess_cbuf_data.fog_type = static_cast<int32>(renderer_settings.fog_type);
		postprocess_cbuf_data.fog_start = renderer_settings.fog_start;
		postprocess_cbuf_data.fog_color = XMVectorSet(renderer_settings.fog_color[0], renderer_settings.fog_color[1], renderer_settings.fog_color[2], 1);
		postprocess_cbuffer->Update(context, postprocess_cbuf_data);

		ID3D11ShaderResourceView* srv_array[] = { postprocess_textures[!postprocess_index]->SRV(), depth_target->SRV() };

		context->PSSetShaderResources(0, ARRAYSIZE(srv_array), srv_array);
		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		ShaderManager::GetShaderProgram(ShaderProgram::Fog)->Bind(context);
		context->Draw(4, 0);

		static ID3D11ShaderResourceView* const srv_null[] = { nullptr, nullptr };
		context->PSSetShaderResources(0, ARRAYSIZE(srv_null), srv_null);
	}

	void Renderer::PassToneMap()
	{
		ID3D11DeviceContext* context = gfx->Context();
		AdriaGfxProfileCondScope(context, "Tone Map Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Tone Map Pass");
		
		postprocess_cbuf_data.tone_map_exposure = renderer_settings.tone_map_exposure;
		postprocess_cbuffer->Update(context, postprocess_cbuf_data);

		ID3D11ShaderResourceView* const srv_array[1] = { postprocess_textures[!postprocess_index]->SRV() };
		static ID3D11ShaderResourceView* const srv_null[1] = { nullptr };

		context->PSSetShaderResources(0, ARRAYSIZE(srv_array), srv_array);
		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		switch (renderer_settings.tone_map_op)
		{
		case ToneMap::Reinhard:
			ShaderManager::GetShaderProgram(ShaderProgram::ToneMap_Reinhard)->Bind(context);
			break;
		case ToneMap::Linear:
			ShaderManager::GetShaderProgram(ShaderProgram::ToneMap_Linear)->Bind(context);
			break;
		case ToneMap::Hable:
			ShaderManager::GetShaderProgram(ShaderProgram::ToneMap_Hable)->Bind(context);
			break;
		default:
			ADRIA_ASSERT(false && "Unsupported Basic effect!");
		}

		context->Draw(4, 0);
		context->PSSetShaderResources(0, ARRAYSIZE(srv_null), srv_null);
	}
	void Renderer::PassFXAA()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.anti_aliasing & EAntiAliasing_FXAA);
		AdriaGfxProfileCondScope(context, "FXAA Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"FXAA Pass");

		static ID3D11ShaderResourceView* const srv_null[] = { nullptr };
		ID3D11ShaderResourceView* srv_array[1] = { fxaa_texture->SRV() };
		context->PSSetShaderResources(0, ARRAYSIZE(srv_array), srv_array);
		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		ShaderManager::GetShaderProgram(ShaderProgram::FXAA)->Bind(context);
		context->Draw(4, 0);
		context->PSSetShaderResources(0, ARRAYSIZE(srv_null), srv_null);
	}
	void Renderer::PassTAA()
	{
		ID3D11DeviceContext* context = gfx->Context();
		ADRIA_ASSERT(renderer_settings.anti_aliasing & EAntiAliasing_TAA);
		AdriaGfxProfileCondScope(context, "TAA Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"TAA Pass");
		
		static ID3D11ShaderResourceView* const srv_null[] = { nullptr, nullptr, nullptr };
		ID3D11ShaderResourceView* srv_array[] = { postprocess_textures[!postprocess_index]->SRV(), prev_hdr_render_target->SRV(), velocity_buffer->SRV() };
		
		context->PSSetShaderResources(0, ARRAYSIZE(srv_array), srv_array);
		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		ShaderManager::GetShaderProgram(ShaderProgram::TAA)->Bind(context);
		context->Draw(4, 0);
		context->PSSetShaderResources(0, ARRAYSIZE(srv_null), srv_null);
	}

	void Renderer::DrawSun(entity sun)
	{
		ID3D11DeviceContext* context = gfx->Context();
		AdriaGfxProfileCondScope(context, "Sun Pass", profiling_enabled);
		AdriaGfxScopedAnnotation(gfx->Annotation(), L"Sun Pass");

		ID3D11RenderTargetView* rtv = sun_target->RTV();
		ID3D11DepthStencilView* dsv = depth_target->DSV();
		
		float black[4] = { 0.0f };
		context->ClearRenderTargetView(rtv, black);

		context->OMSetRenderTargets(1, &rtv, dsv);
		context->OMSetBlendState(alpha_blend.Get(), nullptr, 0xffffffff);
		{
			auto [transform, mesh, material] = reg.get<Transform, Mesh, Material>(sun);
			ShaderManager::GetShaderProgram(ShaderProgram::Sun)->Bind(context);

			object_cbuf_data.model = transform.current_transform;
			object_cbuf_data.transposed_inverse_model = object_cbuf_data.model.Invert();
			object_cbuffer->Update(context, object_cbuf_data);
			material_cbuf_data.diffuse = material.diffuse;
			material_cbuf_data.albedo_factor = material.albedo_factor;
			material_cbuffer->Update(context, material_cbuf_data);

			if (material.albedo_texture != INVALID_TEXTURE_HANDLE)
			{
				auto view = g_TextureManager.GetTextureView(material.albedo_texture);
				context->PSSetShaderResources(TEXTURE_SLOT_DIFFUSE, 1, &view);
			}
			mesh.Draw(context);
		}
		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		context->OMSetRenderTargets(0, nullptr, nullptr);
	}

	void Renderer::BlurTexture(GfxTexture const* src)
	{
		ID3D11DeviceContext* context = gfx->Context();
		std::array<ID3D11UnorderedAccessView*, 2> uavs{};
		std::array<ID3D11ShaderResourceView*, 2>  srvs{};
		
		uavs[0] = blur_texture_intermediate->UAV();  
		uavs[1] = blur_texture_final->UAV();  
		srvs[0] = blur_texture_intermediate->SRV();  
		srvs[1] = blur_texture_final->SRV(); 

		static ID3D11UnorderedAccessView* const null_uav[1] = { nullptr };
		static ID3D11ShaderResourceView* const  null_srv[1] = { nullptr };

		ID3D11UnorderedAccessView* const blur_uav[1] = { nullptr };
		ID3D11ShaderResourceView* const  blur_srv[1] = { nullptr };

		uint32 width = src->GetDesc().width;
		uint32 height = src->GetDesc().height;
		
		ID3D11ShaderResourceView* src_srv = src->SRV();

		context->CSSetShaderResources(0, 1, &src_srv);
		context->CSSetUnorderedAccessViews(0, 1, &uavs[0], nullptr);
		ShaderManager::GetShaderProgram(ShaderProgram::Blur_Horizontal)->Bind(context);
		context->Dispatch((uint32)std::ceil(width * 1.0f / 1024), height, 1);

		//unbind intermediate from uav
		context->CSSetShaderResources(0, 1, null_srv);
		context->CSSetUnorderedAccessViews(0, 1, null_uav, nullptr);

		//and set as srv
		context->CSSetShaderResources(0, 1, &srvs[0]);
		context->CSSetUnorderedAccessViews(0, 1, &uavs[1], nullptr);
		ShaderManager::GetShaderProgram(ShaderProgram::Blur_Vertical)->Bind(context);

		context->Dispatch(width, (uint32)std::ceil(height * 1.0f / 1024), 1);
		//unbind destination as uav
		context->CSSetUnorderedAccessViews(0, 1, null_uav, nullptr);
		context->CSSetShaderResources(0, 1, null_srv);

	}
	void Renderer::CopyTexture(GfxTexture const* src)
	{
		ID3D11DeviceContext* context = gfx->Context();

		ID3D11ShaderResourceView* srv[] = { src->SRV() };
		context->PSSetShaderResources(0, 1, srv);
		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		ShaderManager::GetShaderProgram(ShaderProgram::Copy)->Bind(context);
		context->Draw(4, 0);
		static ID3D11ShaderResourceView* const srv_null[] = { nullptr };
		context->PSSetShaderResources(0, 1, srv_null);
	}
	void Renderer::AddTextures(GfxTexture const* src1, GfxTexture const* src2)
	{
		ID3D11DeviceContext* context = gfx->Context();
		ID3D11ShaderResourceView* srv[] = { src1->SRV(), src2->SRV() };
		context->PSSetShaderResources(0, ARRAYSIZE(srv), srv);
		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		ShaderManager::GetShaderProgram(ShaderProgram::Add)->Bind(context);
		context->Draw(4, 0);
		static ID3D11ShaderResourceView* const srv_null[] = { nullptr, nullptr };
		context->PSSetShaderResources(0, ARRAYSIZE(srv_null), srv_null);
	}

	void Renderer::ResolveCustomRenderState(RenderState const& state, bool reset)
	{
		ID3D11DeviceContext* context = gfx->Context();

		if (reset)
		{
			if(state.blend_state != BlendState::None) context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
			if(state.depth_state != DepthState::None) context->OMSetDepthStencilState(nullptr, 0);
			if(state.raster_state != RasterizerState::None) context->RSSetState(nullptr);
			return;
		}

		switch (state.blend_state)
		{
			case BlendState::AlphaToCoverage:
			{
				FLOAT factors[4] = {};
				context->OMSetBlendState(alpha_to_coverage.Get(), factors, 0xffffffff);
				break;
			}
			case BlendState::AlphaBlend:
			{
				context->OMSetBlendState(alpha_blend.Get(), nullptr, 0xffffffff);
				break;
			}
			case BlendState::AdditiveBlend:
			{
				context->OMSetBlendState(additive_blend.Get(), nullptr, 0xffffffff);
				break;
			}
		}
	}

}

