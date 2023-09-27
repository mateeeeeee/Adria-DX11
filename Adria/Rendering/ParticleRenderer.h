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
		static constexpr uint32 MAX_PARTICLES = 400 * 1024;

		struct GPUParticleA
		{
			Vector4		TintAndAlpha;
			float		Rotation;					
			uint32		IsSleeping;					
		};
		struct GPUParticleB
		{
			Vector3		Position;
			float		Mass;						

			Vector3		Velocity;
			float		Lifespan;					

			float		DistanceToEye;				
			float		Age;						
			float		StartSize;					
			float		EndSize;					
		};
		struct EmitterCBuffer
		{
			Vector4	EmitterPosition;
			Vector4	EmitterVelocity;
			Vector4	PositionVariance;

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
			Vector3 viewspace_position;
			float radius;
		};
		struct SortDispatchInfo
		{
			int32 x, y, z, w;
		};
	public:
		explicit ParticleRenderer(GfxDevice* gfx);

		void Update(float dt, Emitter& emitter_params);

		void Render(Emitter const& emitter_params,
					GfxReadOnlyDescriptor depth_srv, 
					GfxReadOnlyDescriptor particle_srv);

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
		void Simulate(GfxReadOnlyDescriptor depth_srv);
		void Rasterize(Emitter const& emitter_params, GfxReadOnlyDescriptor depth_srv, GfxReadOnlyDescriptor particle_srv);
		void Sort();
		
		bool SortInitial();
		bool SortIncremental(uint32 presorted);
	};
}