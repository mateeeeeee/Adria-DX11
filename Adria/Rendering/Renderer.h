#pragma once
#include <DirectXCollision.h>
#include <memory>
#include <optional>
#include "Picker.h"
#include "ParticleRenderer.h"
#include "RendererSettings.h"
#include "SceneViewport.h"
#include "ConstantBuffers.h"
#include "../tecs/Registry.h"
#include "../Graphics/TextureManager.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/TextureCube.h"
#include "../Graphics/Texture2DArray.h"
#include "../Graphics/ConstantBuffer.h"
#include "../Graphics/ShaderProgram.h"
#include "../Graphics/RenderPass.h"
#include "../Graphics/Profiler.h"
#include "../Graphics/StructuredBuffer.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Texture3D.h"

namespace adria
{

	class Camera;
	class GraphicsCoreDX11;
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

		Renderer(tecs::registry& reg, GraphicsCoreDX11* gfx, uint32 width, uint32 height); 

		void NewFrame(Camera const*);
		void Update(float32 dt);
		
		void SetSceneViewportData(SceneViewport&&);
		void SetProfilerSettings(ProfilerSettings const&);
		void Render(RendererSettings const&);

		void ResolveToOffscreenFramebuffer();
		void ResolveToBackbuffer();

		void OnResize(uint32 width, uint32 height);
		void OnLeftMouseClicked();

		Texture2D GetOffscreenTexture() const;
		TextureManager& GetTextureManager();
		PickingData GetLastPickingData() const;
		std::vector<std::string> GetProfilerResults(bool log = false);

	private:
		uint32 width, height;
		tecs::registry& reg;
		GraphicsCoreDX11* gfx;
		TextureManager texture_manager;
		Camera const* camera;
		RendererSettings renderer_settings;
		Profiler profiler;
		ProfilerSettings profiler_settings;
		ParticleRenderer particle_renderer;

		SceneViewport current_scene_viewport;
		bool pick_in_current_frame = false;
		Picker picker;
		PickingData last_picking_data;

		//textures
		std::vector<Texture2D> gbuffer; 
		Texture2D depth_target;

		Texture2D hdr_render_target;
		Texture2D prev_hdr_render_target;
		Texture2D uav_target;
		Texture2D fxaa_texture;
		Texture2D offscreen_ldr_render_target;
		
		Texture2D shadow_depth_map;
		TextureCube shadow_depth_cubemap;
		Texture2DArray shadow_cascade_maps;
		Texture2D ao_texture;
		Texture2D debug_tiled_texture;
		Texture2D ssao_random_texture;
		Texture2D hbao_random_texture;
		Texture2D blur_texture_intermediate;
		Texture2D blur_texture_final;
		Texture2D bloom_extract_texture;
		std::array<Texture2D, 2> postprocess_textures;
		bool postprocess_index = false;

		std::array<Texture2D, 2> ping_pong_phase_textures;
		bool pong_phase = false;
		std::array<Texture2D, 2> ping_pong_spectrum_textures;
		bool pong_spectrum = false;
		Texture2D ocean_normal_map;
		Texture2D ocean_initial_spectrum;

		Texture3D voxel_texture;
		Texture3D voxel_texture_second_bounce;
		Texture2D sun_target;
		Texture2D velocity_buffer;

		//render passes
		RenderPass gbuffer_pass;
		RenderPass deep_gbuffer_pass;
		RenderPass ambient_pass;
		RenderPass lighting_pass;
		RenderPass forward_pass;
		RenderPass fxaa_pass;
		RenderPass velocity_buffer_pass;
		RenderPass taa_pass;
		RenderPass shadow_map_pass;
		RenderPass ssao_pass;
		RenderPass hbao_pass;
		RenderPass voxel_debug_pass;
		RenderPass offscreen_resolve_pass;
		RenderPass particle_pass;
		RenderPass decal_pass;
		std::array<RenderPass, 6> shadow_cubemap_pass;
		std::vector<RenderPass> cascade_shadow_pass;
		std::array<RenderPass, 2> postprocess_passes; 
		
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
		Microsoft::WRL::ComPtr<ID3D11Buffer> bokeh_buffer;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> bokeh_uav;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bokeh_srv;
		Microsoft::WRL::ComPtr<ID3D11Buffer> bokeh_indirect_draw_buffer;
		TEXTURE_HANDLE hex_bokeh_handle = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE oct_bokeh_handle = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE circle_bokeh_handle = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE cross_bokeh_handle = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE foam_handle = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE perlin_handle = INVALID_TEXTURE_HANDLE;

		//////////////////////////////////////////////////////////////////

		//constant buffers
		FrameCBuffer frame_cbuf_data{};
		std::unique_ptr<ConstantBuffer<FrameCBuffer>> frame_cbuffer = nullptr;
		LightCBuffer light_cbuf_data{};
		std::unique_ptr<ConstantBuffer<LightCBuffer>> light_cbuffer = nullptr;
		ObjectCBuffer object_cbuf_data{};
		std::unique_ptr<ConstantBuffer<ObjectCBuffer>> object_cbuffer = nullptr;
		MaterialCBuffer material_cbuf_data{};
		std::unique_ptr<ConstantBuffer<MaterialCBuffer>> material_cbuffer = nullptr;
		ShadowCBuffer shadow_cbuf_data{};
		std::unique_ptr<ConstantBuffer<ShadowCBuffer>> shadow_cbuffer = nullptr;
		PostprocessCBuffer postprocess_cbuf_data{};
		std::unique_ptr<ConstantBuffer<PostprocessCBuffer>> postprocess_cbuffer = nullptr;
		ComputeCBuffer compute_cbuf_data{};
		std::unique_ptr<ConstantBuffer<ComputeCBuffer>> compute_cbuffer = nullptr;
		WeatherCBuffer weather_cbuf_data{};
		std::unique_ptr<ConstantBuffer<WeatherCBuffer>> weather_cbuffer = nullptr;
		VoxelCBuffer voxel_cbuf_data{};
		std::unique_ptr<ConstantBuffer<VoxelCBuffer>> voxel_cbuffer = nullptr;
		TerrainCBuffer terrain_cbuf_data{};
		std::unique_ptr<ConstantBuffer<TerrainCBuffer>> terrain_cbuffer = nullptr;

		//Structured Buffers
		std::unique_ptr<StructuredBuffer<LightSBuffer>> lights = nullptr;
		std::unique_ptr<StructuredBuffer<VoxelType>>	voxels = nullptr;
		std::unique_ptr<StructuredBuffer<ClusterAABB>>	clusters = nullptr;
		std::unique_ptr<StructuredBuffer<uint32>>			light_counter = nullptr;
		std::unique_ptr<StructuredBuffer<uint32>>			light_list = nullptr;
		std::unique_ptr<StructuredBuffer<LightGrid>>	light_grid = nullptr;

		VertexBuffer cube_vb;
		IndexBuffer cube_ib;

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
		void BlurTexture(Texture2D const& src);
		//result is in currently set render target
		void CopyTexture(Texture2D const& src);
		//result is in currently set render target
		void AddTextures(Texture2D const& src1, Texture2D const& src2);

		void ResolveCustomRenderState(RenderState const&, bool);
	};
}