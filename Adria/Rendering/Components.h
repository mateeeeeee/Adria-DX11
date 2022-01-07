#pragma once
#include <memory>
#include "Enums.h"
#include "../Core/Definitions.h"
#include "../Math/Constants.h"
#include <DirectXCollision.h>
#include "../Graphics/VertexTypes.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/TextureManager.h"
#include "Terrain.h"

#define COMPONENT 

namespace adria
{
	struct COMPONENT Transform
	{
		DirectX::XMMATRIX starting_transform = DirectX::XMMatrixIdentity();
		DirectX::XMMATRIX current_transform = DirectX::XMMatrixIdentity();
	};

	struct COMPONENT Mesh
	{
		std::shared_ptr<VertexBuffer>	vertex_buffer = nullptr;
		std::shared_ptr<IndexBuffer>	index_buffer = nullptr;
		std::shared_ptr<VertexBuffer>   instance_buffer = nullptr;

		//only vb
		U32 vertex_count = 0;
		U32 start_vertex_location = 0; //Index of the first vertex

		//vb/ib
		U32 indices_count = 0;
		U32 start_index_location = 0; //The location of the first index read by the GPU from the index buffer
		I32 base_vertex_location = 0;  //A value added to each index before reading a vertex from the vertex buffer

		//instancing
		U32 instance_count = 0;
		U32 start_instance_location = 0; //A value added to each index before reading per-instance data from a vertex buffer

		D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		void Draw(ID3D11DeviceContext* context) const
		{
			context->IASetPrimitiveTopology(topology);

			vertex_buffer->Bind(context, 0, 0);

			if (index_buffer)
			{
				index_buffer->Bind(context, 0);
				if (instance_buffer)
				{
					instance_buffer->Bind(context, 1);
					context->DrawIndexedInstanced(indices_count, instance_count, 
						start_index_location, base_vertex_location, start_instance_location);
				}
				else context->DrawIndexed(indices_count, start_index_location, base_vertex_location);
			}
			else
			{
				if (instance_buffer)
				{
					instance_buffer->Bind(context, 1);
					context->DrawInstanced(vertex_count, instance_count,
						start_vertex_location, start_instance_location);
				}
				else context->Draw(vertex_count, start_vertex_location);
			}
		}
		void Draw(ID3D11DeviceContext* context, D3D11_PRIMITIVE_TOPOLOGY override_topology) const
		{
			context->IASetPrimitiveTopology(override_topology);

			vertex_buffer->Bind(context, 0, 0);

			if (index_buffer)
			{
				index_buffer->Bind(context, 0);
				if (instance_buffer)
				{
					instance_buffer->Bind(context, 1);
					context->DrawIndexedInstanced(indices_count, instance_count,
						start_index_location, base_vertex_location, start_instance_location);
				}
				else context->DrawIndexed(indices_count, start_index_location, base_vertex_location);
			}
			else
			{
				if (instance_buffer)
				{
					instance_buffer->Bind(context, 1);
					context->DrawInstanced(vertex_count, instance_count,
						start_vertex_location, start_instance_location);
				}
				else context->Draw(vertex_count, start_vertex_location);
			}
		}
	};

	struct COMPONENT Material
	{
		TEXTURE_HANDLE albedo_texture			  = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE normal_texture			  = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE metallic_roughness_texture = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE emissive_texture			  = INVALID_TEXTURE_HANDLE;

		F32 albedo_factor		= 1.0f;
		F32 metallic_factor		= 1.0f;
		F32 roughness_factor	= 1.0f;
		F32 emissive_factor		= 1.0f;

		DirectX::XMFLOAT3 diffuse = DirectX::XMFLOAT3(1, 1, 1);
		EShader shader = EShader::Unknown;
	};

	struct COMPONENT Light
	{
		DirectX::XMVECTOR position	= DirectX::XMVectorSet(0, 10, 0, 1);
		DirectX::XMVECTOR direction	= DirectX::XMVectorSet(0, -1, 0, 0);
		DirectX::XMVECTOR color		= DirectX::XMVectorSet(1, 1, 1, 1);
		F32 energy = 1.0f;
		F32 range = 100.0f;
		ELightType type = ELightType::Directional;
		F32 outer_cosine = cos(pi<F32> / 4);
		F32 inner_cosine = cos(pi<F32> / 8);
		bool casts_shadows = false;
		bool use_cascades = false;
		bool active = true;
		bool volumetric = false;
		F32 volumetric_strength = 1.0f;
		bool screen_space_shadows = false;
		bool lens_flare = false;
		bool god_rays = false;
		F32 godrays_decay = 0.825f;
		F32 godrays_weight = 0.25f;
		F32 godrays_density = 0.975f;
		F32 godrays_exposure = 2.0f;
	};

	struct COMPONENT Visibility
	{
		DirectX::BoundingBox aabb;
		bool camera_visible = true;
		bool light_visible = true;
		bool skip_culling = false;
	};

	struct COMPONENT RenderState
	{
		EBlendState blend_state = EBlendState::None;
		EDepthState depth_state = EDepthState::None;
		ERasterizerState raster_state = ERasterizerState::None;
	};

	struct COMPONENT Skybox 
	{
		TEXTURE_HANDLE cubemap_texture = INVALID_TEXTURE_HANDLE;
		bool active = false;
	};

	struct COMPONENT Ocean {};

	struct COMPONENT Foliage {};

	struct COMPONENT Deferred {};

	struct COMPONENT TerrainComponent
	{
		inline static std::unique_ptr<Terrain> terrain;
		inline static DirectX::XMFLOAT2 texture_scale;
		TEXTURE_HANDLE sand_texture = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE grass_texture = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE rock_texture = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE base_texture = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE layer_texture = INVALID_TEXTURE_HANDLE;
	};

	struct COMPONENT Emitter
	{
		TEXTURE_HANDLE		particle_texture = INVALID_TEXTURE_HANDLE;
		DirectX::XMFLOAT4	position = DirectX::XMFLOAT4(0, 0, 0, 0);
		DirectX::XMFLOAT4	velocity = DirectX::XMFLOAT4(0, 5, 0, 0);
		DirectX::XMFLOAT4	position_variance = DirectX::XMFLOAT4(0, 0, 0, 0);
		I32					number_to_emit = 0;
		F32					particle_lifespan = 5.0f;
		F32					start_size = 10.0f;
		F32					end_size = 1.0f;
		F32					mass = 1.0f;
		F32					velocity_variance = 1.0f;
		F32					particles_per_second = 100;
		F32					accumulation = 0.0f;
		F32					elapsed_time = 0.0f;
		bool				collisions_enabled = false;
		I32					collision_thickness = 40;
		bool				alpha_blended = true;
		bool				pause = false;
		bool				sort = false;
		mutable bool		reset_emitter = true;
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