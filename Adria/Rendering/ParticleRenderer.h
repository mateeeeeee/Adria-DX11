#pragma once
#include <unordered_map>
#include <DirectXMath.h>
#include "Enums.h"
#include "Components.h"
#include "tecs/registry.h"
#include "Graphics/GfxShaderProgram.h"
#include "Graphics/GfxConstantBuffer.h"
#include "Graphics/GfxBuffer.h"
#include "Graphics/GfxTexture.h"
#include "Graphics/GfxDevice.h"

namespace adria
{
	//based on AMD GPU Particles Sample: https://github.com/GPUOpen-LibrariesAndSDKs/GPUParticles11

	class ParticleRenderer
	{
		static constexpr size_t MAX_PARTICLES = 400 * 1024;

		struct GPUParticleA
		{
			DirectX::XMFLOAT4	TintAndAlpha;	
			float		Rotation;					
			uint32		IsSleeping;					
		};
		struct GPUParticleB
		{
			DirectX::XMFLOAT3	Position;		
			float		Mass;						

			DirectX::XMFLOAT3	Velocity;		
			float		Lifespan;					

			float		DistanceToEye;				
			float		Age;						
			float		StartSize;					
			float		EndSize;					
		};
		struct EmitterCBuffer
		{
			DirectX::XMFLOAT4	EmitterPosition;
			DirectX::XMFLOAT4	EmitterVelocity;
			DirectX::XMFLOAT4	PositionVariance;

			int32	MaxParticlesThisFrame;
			float	ParticleLifeSpan;
			float	StartSize;
			float	EndSize;
			
			float	VelocityVariance;
			float	Mass;
			float	ElapsedTime;
			int32 Collisions;

			int32 CollisionThickness;
		};
		struct IndexBufferElement
		{
			float	distance;	
			float	index;		
		};
		struct ViewSpacePositionRadius
		{
			DirectX::XMFLOAT3 viewspace_position;
			float radius;
		};
		struct SortDispatchInfo
		{
			int32 x, y, z, w;
		};
	public:
		ParticleRenderer(GfxDevice* gfx);

		void Update(float dt, Emitter& emitter_params);

		void Render(Emitter const& emitter_params,
					ID3D11ShaderResourceView* depth_srv, 
					ID3D11ShaderResourceView* particle_srv);

	private:
		GfxDevice* gfx;

		std::unique_ptr<GfxTexture> random_texture;
		GfxBuffer dead_list_buffer;
		GfxBuffer particle_bufferA;
		GfxBuffer particle_bufferB;
		GfxBuffer view_space_positions_buffer;
		GfxBuffer alive_index_buffer;

		GfxConstantBuffer<uint32> dead_list_count_cbuffer;
		GfxConstantBuffer<uint32> active_list_count_cbuffer;
		GfxConstantBuffer<EmitterCBuffer> emitter_cbuffer;
		GfxConstantBuffer<SortDispatchInfo> sort_dispatch_info_cbuffer;

		GfxBuffer indirect_render_args_buffer;
		GfxBuffer indirect_sort_args_buffer;
		std::unique_ptr<GfxBuffer> index_buffer;

	private:
		void CreateViews();
		void CreateIndexBuffer();
		void CreateRandomTexture();

		void InitializeDeadList();
		void ResetParticles();
		void Emit(Emitter const& emitter_params);
		void Simulate(ID3D11ShaderResourceView* depth_srv);
		void Rasterize(Emitter const& emitter_params, ID3D11ShaderResourceView* depth_srv, ID3D11ShaderResourceView* particle_srv);
		void Sort();
		
		bool SortInitial();
		bool SortIncremental(uint32 presorted);
	};
}