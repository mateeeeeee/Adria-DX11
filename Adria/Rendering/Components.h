#pragma once
#include <memory>
#include "Enums.h"
#include "../Core/Definitions.h"
#include "../Math/Constants.h"
#include <DirectXCollision.h>
#include "../Utilities/Heightmap.h"

#include "../Graphics/VertexTypes.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/TextureManager.h"
#include <unordered_map>


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
		std::shared_ptr<VertexBuffer>	vb = nullptr;
		std::shared_ptr<IndexBuffer>	ib = nullptr;
		u32 indices_count = 0;
		u32 start_index_location = 0;
		u32 vertex_count = 0;
		u32 vertex_offset = 0;
		D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		void Draw(ID3D11DeviceContext* context) const
		{
			context->IASetPrimitiveTopology(topology);

			vb->Bind(context, 0, 0);

			if (ib)
			{
				ib->Bind(context, 0);
				context->DrawIndexed(indices_count, start_index_location, vertex_offset);
			}
			else context->Draw(vertex_count, vertex_offset);
		}
		void Draw(ID3D11DeviceContext* context, D3D11_PRIMITIVE_TOPOLOGY override_topology) const
		{
			context->IASetPrimitiveTopology(override_topology);

			vb->Bind(context, 0, 0);

			if (ib)
			{
				ib->Bind(context, 0);
				context->DrawIndexed(indices_count, start_index_location, vertex_offset);
			}
			else context->Draw(vertex_count, vertex_offset);
		}
	};

	struct COMPONENT InstanceComponent
	{
		VertexBuffer instance_buffer;
		u32 instance_count = 0;
		u32 start_instance_location = 0;
	};

	struct COMPONENT Material
	{
		TEXTURE_HANDLE albedo_texture			  = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE normal_texture			  = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE metallic_roughness_texture = INVALID_TEXTURE_HANDLE;
		TEXTURE_HANDLE emissive_texture			  = INVALID_TEXTURE_HANDLE;

		f32 albedo_factor		= 1.0f;
		f32 metallic_factor		= 1.0f;
		f32 roughness_factor	= 1.0f;
		f32 emissive_factor		= 1.0f;

		DirectX::XMFLOAT3 diffuse = DirectX::XMFLOAT3(1, 1, 1);
		EShader shader = EShader::Unknown;
	};

	struct COMPONENT Light
	{
		DirectX::XMVECTOR position	= DirectX::XMVectorSet(0, 10, 0, 1);
		DirectX::XMVECTOR direction	= DirectX::XMVectorSet(0, -1, 0, 0);
		DirectX::XMVECTOR color		= DirectX::XMVectorSet(1, 1, 1, 1);
		f32 energy = 1.0f;
		f32 range = 100.0f;
		ELightType type = ELightType::Directional;
		f32 outer_cosine = cos(pi<f32> / 4);
		f32 inner_cosine = cos(pi<f32> / 8);
		bool casts_shadows = false;
		bool use_cascades = false;
		bool active = true;
		bool volumetric = false;
		f32 volumetric_strength = 1.0f;
		bool screen_space_shadows = false;
		bool lens_flare = false;
		bool god_rays = false;
		f32 godrays_decay = 0.825f;
		f32 godrays_weight = 0.25f;
		f32 godrays_density = 0.975f;
		f32 godrays_exposure = 2.0f;
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

	struct COMPONENT Deferred {};

	struct COMPONENT Forward 
	{
		bool transparent;
	};

	struct COMPONENT Tag
	{
		std::string name = "default";
	};
}