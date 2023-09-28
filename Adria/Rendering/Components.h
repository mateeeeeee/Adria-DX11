#pragma once
#include <memory>
#include "Enums.h"
#include "Terrain.h"
#include "TextureManager.h"
#include "Core/CoreTypes.h"
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
		uint32 children_count = 0;
		tecs::entity children[MAX_CHILDREN] = { tecs::null_entity };
	};

	struct COMPONENT Mesh
	{
		std::shared_ptr<GfxBuffer>	vertex_buffer = nullptr;
		std::shared_ptr<GfxBuffer>	index_buffer = nullptr;
		std::shared_ptr<GfxBuffer>   instance_buffer = nullptr;

		//only vb
		uint32 vertex_count = 0;
		uint32 start_vertex_location = 0; //Index of the first vertex

		//vb/ib
		uint32 indices_count = 0;
		uint32 start_index_location = 0; //The location of the first index read by the GPU from the index buffer
		int32 base_vertex_location = 0;  //A value added to each index before reading a vertex from the vertex buffer

		//instancing
		uint32 instance_count = 1;
		uint32 start_instance_location = 0; //A value added to each index before reading per-instance data from a vertex buffer

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

		float albedo_factor		= 1.0f;
		float metallic_factor		= 1.0f;
		float roughness_factor	= 1.0f;
		float emissive_factor		= 1.0f;

		MaterialAlphaMode alpha_mode = MaterialAlphaMode::Opaque;
		float alpha_cutoff		= 0.5f;
		bool    double_sided		= false;

		Vector3 diffuse = Vector3(1, 1, 1);
		ShaderProgram shader = ShaderProgram::Unknown;
	};

	struct COMPONENT Light
	{
		Vector4 position = Vector4(0, 10, 0, 1);
		Vector4 direction = Vector4(0, -1, 0, 0);
		Vector4 color = Vector4(1, 1, 1, 1);
		float energy = 1.0f;
		float range = 100.0f;
		LightType type = LightType::Directional;
		float outer_cosine = cos(pi<float> / 4);
		float inner_cosine = cos(pi<float> / 8);
		bool casts_shadows = false;
		bool use_cascades = false;
		bool active = true;
		bool volumetric = false;
		float volumetric_strength = 1.0f;
		bool screen_space_contact_shadows = false;
		float sscs_thickness = 0.5f;
		float sscs_max_ray_distance = 0.05f;
		float sscs_max_depth_distance = 200.0f;
		bool lens_flare = false;
		bool god_rays = false;
		float godrays_decay = 0.825f;
		float godrays_weight = 0.25f;
		float godrays_density = 0.975f;
		float godrays_exposure = 2.0f;
	};

	struct COMPONENT AABB
	{
		BoundingBox bounding_box;
		bool camera_visible = true;
		bool light_visible = true;
		bool skip_culling = false;
		bool draw_aabb = false;
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
		bool active = false;
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
		int32					number_to_emit = 0;
		float					particle_lifespan = 5.0f;
		float					start_size = 10.0f;
		float					end_size = 1.0f;
		float					mass = 1.0f;
		float					velocity_variance = 1.0f;
		float					particles_per_second = 100;
		float					accumulation = 0.0f;
		float					elapsed_time = 0.0f;
		bool				collisions_enabled = false;
		int32					collision_thickness = 40;
		bool				alpha_blended = true;
		bool				pause = false;
		bool				sort = false;
		mutable bool		reset_emitter = true;
	};

	struct COMPONENT Decal
	{
		TextureHandle albedo_decal_texture = INVALID_TEXTURE_HANDLE;
		TextureHandle normal_decal_texture = INVALID_TEXTURE_HANDLE;
		Matrix decal_model_matrix = Matrix::Identity;
		DecalType decal_type = DecalType::Project_XY;
		bool modify_gbuffer_normals = false;
	};

	struct COMPONENT Forward 
	{
		bool transparent;
	};

	struct COMPONENT Tag
	{
		std::string name = "default";
	};
}