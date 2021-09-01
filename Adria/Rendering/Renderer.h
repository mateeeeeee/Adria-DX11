#pragma once
#include <DirectXCollision.h>
#include <memory>
#include <optional>
#include "RendererSettings.h"
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
#include "../Graphics/Texture3D.h"

namespace adria
{

	class Camera;
	class GraphicsCoreDX11;
	struct Light;
	

	class Renderer
	{
		static constexpr u32 AO_NOISE_DIM = 8;
		static constexpr u32 SSAO_KERNEL_SIZE = 16;
		static constexpr u32 RESOLUTION = 512;
		static constexpr u32 VOXEL_RESOLUTION = 128;
		static constexpr u32 VOXELIZE_MAX_LIGHTS = 8;
		static constexpr u32 CLUSTER_SIZE_X = 16;
		static constexpr u32 CLUSTER_SIZE_Y = 16;
		static constexpr u32 CLUSTER_SIZE_Z = 16;
		static constexpr u32 CLUSTER_MAX_LIGHTS = 128;
		static constexpr DXGI_FORMAT GBUFFER_FORMAT[GBUFFER_SIZE] = { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM };

	public:

		Renderer(tecs::registry& reg, GraphicsCoreDX11* gfx, u32 width, u32 height); 

		void NewFrame(Camera const*);

		void Update(f32 dt);

		void SetProfilerSettings(ProfilerFlags);

		void Render(RendererSettings const&);

		void ResolveToOffscreenFramebuffer();

		void ResolveToBackbuffer();

		void OnResize(u32 width, u32 height);

		Texture2D GetOffscreenTexture() const;

		TextureManager& GetTextureManager();

		Profiler& GetProfiler();

	private:
		u32 width, height;
		tecs::registry& reg;
		GraphicsCoreDX11* gfx;
		TextureManager texture_manager;
		Camera const* camera;
		RendererSettings renderer_settings;
		Profiler profiler;
		ProfilerFlags profiler_flags;

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
		Texture2D ocean_initial_spectrum;
		std::array<Texture2D, 2> ping_pong_phase_textures;
		bool pong_phase_pass = false;
		std::array<Texture2D, 2> ping_pong_spectrum_textures;
		bool pong_spectrum = true;
		Texture3D voxel_texture;
		Texture3D voxel_texture_second_bounce;
		Texture2D sun_target;
		Texture2D velocity_buffer;

		//shaders
		std::unordered_map<StandardShader, StandardProgram> standard_programs;
		std::unordered_map<ComputeShader, ComputeProgram>  compute_programs;
		std::unordered_map<GeometryShader, GeometryProgram> geometry_programs;
		std::unordered_map<TesselationShader, TessellationProgram> tesselation_programs;
		
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
		Texture2D ocean_normal_map;
		
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

		//Structured Buffers
		std::unique_ptr<StructuredBuffer<LightSBuffer>> lights = nullptr;
		std::unique_ptr<StructuredBuffer<VoxelType>>	voxels = nullptr;
		std::unique_ptr<StructuredBuffer<ClusterAABB>>	clusters = nullptr;
		std::unique_ptr<StructuredBuffer<u32>>			light_counter = nullptr;
		std::unique_ptr<StructuredBuffer<u32>>			light_list = nullptr;
		std::unique_ptr<StructuredBuffer<LightGrid>>	light_grid = nullptr;

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
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		leq_depth;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		scissor_enabled;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		cull_none;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		shadow_depth_bias;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		wireframe;

	private:
		//called in constructor
		void LoadShaders();
		void LoadTextures();
		void AddProfilerBlocks();
		void CreateBuffers();
		void CreateSamplers();
		void CreateRenderStates();
		void CreateBokehViews(u32 width, u32 height);
		void CreateRenderTargets(u32 width, u32 height);
		void CreateGBuffer(u32 width, u32 height);
		void CreateSsaoTextures(u32 width, u32 height);
		void CreateRenderPasses(u32 width, u32 height);
		void CreateComputeTextures(u32 width, u32 height);
		void CreateIBLTextures();

		//called in update
		void BindGlobals();
		void UpdateSkybox();
		void UpdateOcean(f32 dt);
		void UpdateWeather(f32 dt);
		void UpdateLights();
		void UpdateVoxelData();
		void CameraFrustumCulling();
		void LightFrustumCulling(LightType type);
		
		//called in render
		void PassGBuffer();
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
		void PassVolumetric(Light const& light);
		

		//called in forward pass
		void PassSkybox();
		void PassOcean();
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
	};
}