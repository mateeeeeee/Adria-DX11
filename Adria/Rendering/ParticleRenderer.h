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
		static constexpr Uint32 MAX_PARTICLES = 400 * 1024;

		struct GPUParticleA
		{
			Vector4		TintAndAlpha;
			Float		Rotation;					
			Uint32		IsSleeping;					
		};
		struct GPUParticleB
		{
			Vector3		Position;
			Float		Mass;						

			Vector3		Velocity;
			Float		Lifespan;					

			Float		DistanceToEye;				
			Float		Age;						
			Float		StartSize;					
			Float		EndSize;					
		};
		struct EmitterCBuffer
		{
			Vector4	EmitterPosition;
			Vector4	EmitterVelocity;
			Vector4	PositionVariance;

			Sint32	MaxParticlesThisFrame;
			Float	ParticleLifeSpan;
			Float	StartSize;
			Float	EndSize;
			
			Float	VelocityVariance;
			Float	Mass;
			Float	ElapsedTime;
			Sint32 Collisions;

			Sint32 CollisionThickness;
		};
		struct IndexBufferElement
		{
			Float	distance;	
			Float	index;		
		};
		struct ViewSpacePositionRadius
		{
			Vector3 viewspace_position;
			Float radius;
		};
		struct SortDispatchInfo
		{
			Sint32 x, y, z, w;
		};
	public:
		explicit ParticleRenderer(GfxDevice* gfx);

		void Update(Float dt, Emitter& emitter_params);

		void Render(Emitter const& emitter_params,
					GfxShaderResourceRO depth_srv, 
					GfxShaderResourceRO particle_srv);

	private:
		GfxDevice* gfx;

		std::unique_ptr<GfxTexture> random_texture;
		GfxBuffer dead_list_buffer;
		GfxBuffer particle_bufferA;
		GfxBuffer particle_bufferB;
		GfxBuffer view_space_positions_buffer;
		GfxBuffer alive_index_buffer;

		GfxConstantBuffer<Uint32> dead_list_count_cbuffer;
		GfxConstantBuffer<Uint32> active_list_count_cbuffer;
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
		void Simulate(GfxShaderResourceRO depth_srv);
		void Rasterize(Emitter const& emitter_params, GfxShaderResourceRO depth_srv, GfxShaderResourceRO particle_srv);
		void Sort();
		
		Bool SortInitial();
		Bool SortIncremental(Uint32 presorted);
	};
}