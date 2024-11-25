#pragma once
#include <memory>
#include "Enums.h"
#include "Terrain.h"
#include "TextureManager.h"
#include "Math/Constants.h"
#include "Graphics/GfxVertexFormat.h"
#include "Graphics/GfxBuffer.h"
#include "Graphics/GfxStates.h"
#include "tecs/entity.h"

#define COMPONENT 

namespace adria
{
	class GfxCommandContext;

	struct COMPONENT Transform
	{
		Matrix starting_transform = Matrix::Identity;
		Matrix current_transform = Matrix::Identity;
	};

	struct COMPONENT Relationship
	{
		static constexpr size_t MAX_CHILDREN = 2048;
		tecs::entity parent = tecs::null_entity;
		Uint32 children_count = 0;
		tecs::entity children[MAX_CHILDREN] = { tecs::null_entity };
	};

	struct COMPONENT Mesh
	{
		std::shared_ptr<GfxBuffer>	vertex_buffer = nullptr;
		std::shared_ptr<GfxBuffer>	index_buffer = nullptr;
		std::shared_ptr<GfxBuffer>   instance_buffer = nullptr;

		//only vb
		Uint32 vertex_count = 0;
		Uint32 start_vertex_location = 0; //Index of the first vertex

		//vb/ib
		Uint32 indices_count = 0;
		Uint32 start_index_location = 0; //The location of the first index read by the GPU from the index buffer
		Int32 base_vertex_location = 0;  //A value added to each index before reading a vertex from the vertex buffer

		//instancing
		Uint32 instance_count = 1;
		Uint32 start_instance_location = 0; //A value added to each index before reading per-instance data from a vertex buffer

		GfxPrimitiveTopology topology = GfxPrimitiveTopology::TriangleList;

		void Draw(GfxCommandContext* context) const;
		void Draw(GfxCommandContext* context, GfxPrimitiveTopology override_topology) const;
	};

	struct COMPONENT Material
	{
		TextureHandle albedo_texture			  = INVALID_TEXTURE_HANDLE;
		TextureHandle normal_texture			  = INVALID_TEXTURE_HANDLE;
		TextureHandle metallic_roughness_texture  = INVALID_TEXTURE_HANDLE;
		TextureHandle emissive_texture			  = INVALID_TEXTURE_HANDLE;

		Float albedo_factor		= 1.0f;
		Float metallic_factor		= 1.0f;
		Float roughness_factor	= 1.0f;
		Float emissive_factor		= 1.0f;

		MaterialAlphaMode alpha_mode = MaterialAlphaMode::Opaque;
		Float alpha_cutoff		= 0.5f;
		Bool    double_sided		= false;

		Vector3 diffuse = Vector3(1, 1, 1);
		ShaderProgram shader = ShaderProgram::Unknown;
	};

	struct COMPONENT Light
	{
		Vector4 position = Vector4(0, 10, 0, 1);
		Vector4 direction = Vector4(0, -1, 0, 0);
		Vector4 color = Vector4(1, 1, 1, 1);
		Float energy = 1.0f;
		Float range = 100.0f;
		LightType type = LightType::Directional;
		Float outer_cosine = cos(pi<Float> / 4);
		Float inner_cosine = cos(pi<Float> / 8);
		Bool casts_shadows = false;
		Bool use_cascades = false;
		Bool active = true;
		Bool volumetric = false;
		Float volumetric_strength = 0.03f;
		Bool screen_space_contact_shadows = false;
		Float sscs_thickness = 0.5f;
		Float sscs_max_ray_distance = 0.05f;
		Float sscs_max_depth_distance = 200.0f;
		Bool lens_flare = false;
		Bool god_rays = false;
		Float godrays_decay = 0.825f;
		Float godrays_weight = 0.25f;
		Float godrays_density = 0.975f;
		Float godrays_exposure = 2.0f;
	};

	struct COMPONENT AABB
	{
		BoundingBox bounding_box;
		Bool camera_visible = true;
		Bool light_visible = true;
		Bool skip_culling = false;
		Bool draw_aabb = false;
		std::shared_ptr<GfxBuffer> aabb_vb = nullptr;

		void UpdateBuffer(GfxDevice* gfx)
		{
			Vector3 corners[8];
			bounding_box.GetCorners(corners);
			SimpleVertex vertices[] =
			{
				SimpleVertex{corners[0]},
				SimpleVertex{corners[1]},
				SimpleVertex{corners[2]},
				SimpleVertex{corners[3]},
				SimpleVertex{corners[4]},
				SimpleVertex{corners[5]},
				SimpleVertex{corners[6]},
				SimpleVertex{corners[7]}
			}; 
			aabb_vb = std::make_unique<GfxBuffer>(gfx, VertexBufferDesc(ARRAYSIZE(vertices), sizeof(SimpleVertex)), vertices);
		}
	};

	struct COMPONENT RenderState
	{
		BlendState blend_state = BlendState::None;
		DepthState depth_state = DepthState::None;
		RasterizerState raster_state = RasterizerState::None;
	};

	struct COMPONENT Skybox 
	{
		TextureHandle cubemap_texture = INVALID_TEXTURE_HANDLE;
		Bool active = false;
	};

	struct COMPONENT Ocean {};

	struct COMPONENT Foliage {};

	struct COMPONENT Deferred {};

	struct COMPONENT TerrainComponent
	{
		inline static std::unique_ptr<Terrain> terrain;
		inline static Vector2 texture_scale;
		TextureHandle sand_texture = INVALID_TEXTURE_HANDLE;
		TextureHandle grass_texture = INVALID_TEXTURE_HANDLE;
		TextureHandle rock_texture = INVALID_TEXTURE_HANDLE;
		TextureHandle base_texture = INVALID_TEXTURE_HANDLE;
		TextureHandle layer_texture = INVALID_TEXTURE_HANDLE;
	};

	struct COMPONENT Emitter
	{
		TextureHandle		particle_texture = INVALID_TEXTURE_HANDLE;
		Vector4	position		  = Vector4(0, 0, 0, 0);
		Vector4	velocity		  = Vector4(0, 5, 0, 0);
		Vector4	position_variance = Vector4(0, 0, 0, 0);
		Int32					number_to_emit = 0;
		Float					particle_lifespan = 5.0f;
		Float					start_size = 10.0f;
		Float					end_size = 1.0f;
		Float					mass = 1.0f;
		Float					velocity_variance = 1.0f;
		Float					particles_per_second = 100;
		Float					accumulation = 0.0f;
		Float					elapsed_time = 0.0f;
		Bool				collisions_enabled = false;
		Int32					collision_thickness = 40;
		Bool				alpha_blended = true;
		Bool				pause = false;
		Bool				sort = false;
		mutable Bool		reset_emitter = true;
	};

	struct COMPONENT Decal
	{
		TextureHandle albedo_decal_texture = INVALID_TEXTURE_HANDLE;
		TextureHandle normal_decal_texture = INVALID_TEXTURE_HANDLE;
		Matrix decal_model_matrix = Matrix::Identity;
		DecalType decal_type = DecalType::Project_XY;
		Bool modify_gbuffer_normals = false;
	};

	struct COMPONENT Forward 
	{
		Bool transparent;
	};

	struct COMPONENT Tag
	{
		std::string name = "default";
	};
}