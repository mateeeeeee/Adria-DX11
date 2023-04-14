#pragma once
#include <DirectXCollision.h>
#include <memory>
#include <optional>
#include "Picker.h"
#include "ParticleRenderer.h"
#include "RendererSettings.h"
#include "SceneViewport.h"
#include "ConstantBuffers.h"
#include "TextureManager.h"
#include "../Graphics/GfxConstantBuffer.h"
#include "../Graphics/GfxShaderProgram.h"
#include "../Graphics/GfxRenderPass.h"
#include "../Graphics/GPUProfiler.h"
#include "../Graphics/GfxBuffer.h"
#include "../tecs/Registry.h"

namespace adria
{

	class Camera;
	class GfxDevice;
	class Input;
	struct Light;
	struct RenderState;

	class Renderer
	{
		static constexpr uint32 AO_NOISE_DIM = 8;
		static constexpr uint32 SSAO_KERNEL_SIZE = 16;
		static constexpr uint32 RESOLUTION = 512;
		static constexpr uint32 VOXEL_RESOLUTION = 128;
		static constexpr uint32 VOXELIZE_MAX_LIGHTS = 8;
		static constexpr uint32 CLUSTER_SIZE_X = 16;
		static constexpr uint32 CLUSTER_SIZE_Y = 16;
		static constexpr uint32 CLUSTER_SIZE_Z = 16;
		static constexpr uint32 CLUSTER_MAX_LIGHTS = 128;
		static constexpr DXGI_FORMAT GBUFFER_FORMAT[EGBufferSlot_Count] = { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM };

	public:

		Renderer(tecs::registry& reg, GfxDevice* gfx, uint32 width, uint32 height); 
		~Renderer();

		void NewFrame(Camera const*);
		void Update(float32 dt);
		
		void SetSceneViewportData(SceneViewport&&);
		void SetProfilerSettings(GPUProfilerSettings const&);
		void Render(RendererSettings const&);

		void ResolveToOffscreenFramebuffer();
		void ResolveToBackbuffer();

		void OnResize(uint32 width, uint32 height);
		void OnLeftMouseClicked();

		GfxTexture const* GetOffscreenTexture() const;
		PickingData GetLastPickingData() const;
		std::vector<Timestamp> GetProfilerResults();

	private:
		uint32 width, height;
		tecs::registry& reg;
		GfxDevice* gfx;
		Camera const* camera;
		RendererSettings renderer_settings;
		GPUProfiler profiler;
		GPUProfilerSettings profiler_settings;
		ParticleRenderer particle_renderer;

		SceneViewport current_scene_viewport;
		bool pick_in_current_frame = false;
		Picker picker;
		PickingData last_picking_data;

		//textures
		std::vector<std::unique_ptr<GfxTexture>> gbuffer;
		std::unique_ptr<GfxTexture> depth_target;

		std::unique_ptr<GfxTexture> hdr_render_target;
		std::unique_ptr<GfxTexture> prev_hdr_render_target;
		std::unique_ptr<GfxTexture> uav_target;
		std::unique_ptr<GfxTexture> fxaa_texture;
		std::unique_ptr<GfxTexture> offscreen_ldr_render_target;
		
		std::unique_ptr<GfxTexture> shadow_depth_map;
		std::unique_ptr<GfxTexture> shadow_depth_cubemap;
		std::unique_ptr<GfxTexture> shadow_cascade_maps;
		std::unique_ptr<GfxTexture> ao_texture;
		std::unique_ptr<GfxTexture> debug_tiled_texture;
		std::unique_ptr<GfxTexture> ssao_random_texture;
		std::unique_ptr<GfxTexture> hbao_random_texture;
		std::unique_ptr<GfxTexture> blur_texture_intermediate;
		std::unique_ptr<GfxTexture> blur_texture_final;
		std::unique_ptr<GfxTexture> bloom_extract_texture;
		std::array<std::unique_ptr<GfxTexture>, 2> postprocess_textures;
		bool postprocess_index = false;

		std::array<std::unique_ptr<GfxTexture>, 2> ping_pong_phase_textures;
		bool pong_phase = false;
		std::array<std::unique_ptr<GfxTexture>, 2> ping_pong_spectrum_textures;
		bool pong_spectrum = false;
		std::unique_ptr<GfxTexture> ocean_normal_map;
		std::unique_ptr<GfxTexture> ocean_initial_spectrum;

		std::unique_ptr<GfxTexture> voxel_texture;
		std::unique_ptr<GfxTexture> voxel_texture_second_bounce;
		std::unique_ptr<GfxTexture> sun_target;
		std::unique_ptr<GfxTexture> velocity_buffer;

		std::unique_ptr<GfxBuffer> bokeh_buffer;
		std::unique_ptr<GfxBuffer> bokeh_indirect_draw_buffer;

		//render passes
		GfxRenderPass gbuffer_pass;
		GfxRenderPass deep_gbuffer_pass;
		GfxRenderPass ambient_pass;
		GfxRenderPass lighting_pass;
		GfxRenderPass forward_pass;
		GfxRenderPass fxaa_pass;
		GfxRenderPass velocity_buffer_pass;
		GfxRenderPass taa_pass;
		GfxRenderPass shadow_map_pass;
		GfxRenderPass ssao_pass;
		GfxRenderPass hbao_pass;
		GfxRenderPass voxel_debug_pass;
		GfxRenderPass offscreen_resolve_pass;
		GfxRenderPass particle_pass;
		GfxRenderPass decal_pass;
		std::array<GfxRenderPass, 6> shadow_cubemap_pass;
		std::vector<GfxRenderPass> cascade_shadow_pass;
		std::array<GfxRenderPass, 2> postprocess_passes; 
		
		//other
		//////////////////////////////////////////////////////////////////
		bool ibl_textures_generated = false;
		bool recreate_clusters = true;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> env_srv;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> irmap_srv;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> brdf_srv;
		DirectX::BoundingBox light_bounding_box;
		DirectX::BoundingFrustum light_bounding_frustum;
		std::optional<DirectX::BoundingSphere> scene_bounding_sphere = std::nullopt;
		std::array<DirectX::XMVECTOR, SSAO_KERNEL_SIZE> ssao_kernel{};
		std::vector<ID3D11ShaderResourceView*> lens_flare_textures;
		std::vector<ID3D11ShaderResourceView*> clouds_textures;
		TextureHandle hex_bokeh_handle = INVALID_TEXTURE_HANDLE;
		TextureHandle oct_bokeh_handle = INVALID_TEXTURE_HANDLE;
		TextureHandle circle_bokeh_handle = INVALID_TEXTURE_HANDLE;
		TextureHandle cross_bokeh_handle = INVALID_TEXTURE_HANDLE;
		TextureHandle foam_handle = INVALID_TEXTURE_HANDLE;
		TextureHandle perlin_handle = INVALID_TEXTURE_HANDLE;

		//////////////////////////////////////////////////////////////////

		//constant buffers
		FrameCBuffer frame_cbuf_data{};
		std::unique_ptr<GfxConstantBuffer<FrameCBuffer>> frame_cbuffer = nullptr;
		LightCBuffer light_cbuf_data{};
		std::unique_ptr<GfxConstantBuffer<LightCBuffer>> light_cbuffer = nullptr;
		ObjectCBuffer object_cbuf_data{};
		std::unique_ptr<GfxConstantBuffer<ObjectCBuffer>> object_cbuffer = nullptr;
		MaterialCBuffer material_cbuf_data{};
		std::unique_ptr<GfxConstantBuffer<MaterialCBuffer>> material_cbuffer = nullptr;
		ShadowCBuffer shadow_cbuf_data{};
		std::unique_ptr<GfxConstantBuffer<ShadowCBuffer>> shadow_cbuffer = nullptr;
		PostprocessCBuffer postprocess_cbuf_data{};
		std::unique_ptr<GfxConstantBuffer<PostprocessCBuffer>> postprocess_cbuffer = nullptr;
		ComputeCBuffer compute_cbuf_data{};
		std::unique_ptr<GfxConstantBuffer<ComputeCBuffer>> compute_cbuffer = nullptr;
		WeatherCBuffer weather_cbuf_data{};
		std::unique_ptr<GfxConstantBuffer<WeatherCBuffer>> weather_cbuffer = nullptr;
		VoxelCBuffer voxel_cbuf_data{};
		std::unique_ptr<GfxConstantBuffer<VoxelCBuffer>> voxel_cbuffer = nullptr;
		TerrainCBuffer terrain_cbuf_data{};
		std::unique_ptr<GfxConstantBuffer<TerrainCBuffer>> terrain_cbuffer = nullptr;

		std::unique_ptr<GfxBuffer> lights = nullptr;
		std::unique_ptr<GfxBuffer>	voxels = nullptr;
		std::unique_ptr<GfxBuffer> clusters = nullptr;
		std::unique_ptr<GfxBuffer>	light_counter = nullptr;
		std::unique_ptr<GfxBuffer>	light_list = nullptr;
		std::unique_ptr<GfxBuffer>	light_grid = nullptr;

		std::unique_ptr<GfxBuffer> cube_vb;
		std::unique_ptr<GfxBuffer> cube_ib;
		std::unique_ptr<GfxBuffer> aabb_wireframe_ib;

		//samplers
		Microsoft::WRL::ComPtr<ID3D11SamplerState>			linear_wrap_sampler;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>			point_wrap_sampler;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>			linear_border_sampler;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>			linear_clamp_sampler;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>			point_clamp_sampler;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>			shadow_sampler;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>			anisotropic_sampler;
		//render states
		Microsoft::WRL::ComPtr<ID3D11BlendState>			additive_blend;
		Microsoft::WRL::ComPtr<ID3D11BlendState>			alpha_blend;
		Microsoft::WRL::ComPtr<ID3D11BlendState>			alpha_to_coverage;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		leq_depth;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		no_depth_test;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		scissor_enabled;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		cull_none;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		shadow_depth_bias;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		wireframe;

	private:

		void LoadTextures();
		void CreateBuffers();
		void CreateSamplers();
		void CreateRenderStates();
		void CreateResolutionDependentResources(uint32 width, uint32 height);
		void CreateOtherResources();

		void CreateBokehViews(uint32 width, uint32 height);
		void CreateRenderTargets(uint32 width, uint32 height);
		void CreateGBuffer(uint32 width, uint32 height);
		void CreateAOTexture(uint32 width, uint32 height);
		void CreateRenderPasses(uint32 width, uint32 height);
		void CreateComputeTextures(uint32 width, uint32 height);
		void CreateIBLTextures();

		//called in update
		void BindGlobals();

		void UpdateCBuffers(float32 dt);
		void UpdateOcean(float32 dt);
		void UpdateWeather(float32 dt);
		void UpdateParticles(float32 dt);
		void UpdateLights();
		void UpdateTerrainData();
		void UpdateVoxelData();
		void CameraFrustumCulling();
		void LightFrustumCulling(ELightType type);
		
		//called in render
		void PassPicking();
		void PassGBuffer();
		void PassDecals();
		void PassSSAO();
		void PassHBAO();
		void PassAmbient();
		void PassDeferredLighting();  
		void PassDeferredTiledLighting();
		void PassDeferredClusteredLighting();
		void PassForward(); 
		void PassVoxelize();
		void PassVoxelizeDebug();
		void PassVoxelGI();
		void PassPostprocessing();

		void PassShadowMapDirectional(Light const& light);
		void PassShadowMapSpot(Light const& light);
		void PassShadowMapPoint(Light const& light);
		void PassShadowMapCascades(Light const& light);
		void PassShadowMapCommon();
		void PassVolumetric(Light const& light);
		
		//called in forward pass
		void PassSky();
		void PassOcean();
		void PassParticles();
		void PassAABB();
		void PassForwardCommon(bool transparent);
		
		//POSTPROCESS
		
		//called in postprocessing pass
		void PassLensFlare(Light const& light);
		void PassVolumetricClouds();
		void PassSSR(); 
		void PassGodRays(Light const& light); 
		void PassDepthOfField(); 
		void PassBloom(); 
		void PassVelocityBuffer();
		void PassMotionBlur();
		void PassFog();
		void PassFXAA();
		void PassTAA();
		void PassToneMap();

		//result is in sun texture
		void DrawSun(tecs::entity sun);
		//result is in final compute texture
		void BlurTexture(GfxTexture const* src);
		//result is in currently set render target
		void CopyTexture(GfxTexture const* src);
		//result is in currently set render target
		void AddTextures(GfxTexture const* src1, GfxTexture const* src2);

		void ResolveCustomRenderState(RenderState const&, bool);
	};
}