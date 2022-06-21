#pragma once
#include <unordered_map>
#include <DirectXMath.h>
#include "Enums.h"
#include "Components.h"
#include "../tecs/registry.h"
#include "../Graphics/ShaderProgram.h"
#include "../Graphics/ConstantBuffer.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"

#include "../Graphics/Buffer.h"
#include "../Graphics/Texture.h"
#include "../Graphics/GraphicsDeviceDX11.h"

namespace adria
{
	//based on AMD GPU Particles Sample: https://github.com/GPUOpen-LibrariesAndSDKs/GPUParticles11

	class ParticleRenderer
	{
		static constexpr size_t MAX_PARTICLES = 400 * 1024;

		struct GPUParticleA
		{
			DirectX::XMFLOAT4	TintAndAlpha;	
			float32		Rotation;					
			uint32		IsSleeping;					
		};
		struct GPUParticleB
		{
			DirectX::XMFLOAT3	Position;		
			float32		Mass;						

			DirectX::XMFLOAT3	Velocity;		
			float32		Lifespan;					

			float32		DistanceToEye;				
			float32		Age;						
			float32		StartSize;					
			float32		EndSize;					
		};
		struct EmitterCBuffer
		{
			DirectX::XMFLOAT4	EmitterPosition;
			DirectX::XMFLOAT4	EmitterVelocity;
			DirectX::XMFLOAT4	PositionVariance;

			int32	MaxParticlesThisFrame;
			float32	ParticleLifeSpan;
			float32	StartSize;
			float32	EndSize;
			
			float32	VelocityVariance;
			float32	Mass;
			float32	ElapsedTime;
			int32 Collisions;

			int32 CollisionThickness;
		};
		struct IndexBufferElement
		{
			float32	distance;	
			float32	index;		
		};
		struct ViewSpacePositionRadius
		{
			DirectX::XMFLOAT3 viewspace_position;
			float32 radius;
		};
		struct SortDispatchInfo
		{
			int32 x, y, z, w;
		};
	public:
		ParticleRenderer(GraphicsDevice* gfx);

		void Update(float32 dt, Emitter& emitter_params);

		void Render(Emitter const& emitter_params,
					ID3D11ShaderResourceView* depth_srv, 
					ID3D11ShaderResourceView* particle_srv);

	private:
		GraphicsDevice* gfx;

		std::unique_ptr<Texture> random_texture;
		Buffer dead_list_buffer;
		Buffer particle_bufferA;
		Buffer particle_bufferB;
		Buffer view_space_positions_buffer;
		Buffer alive_index_buffer;

		ConstantBuffer<uint32> dead_list_count_cbuffer;
		ConstantBuffer<uint32> active_list_count_cbuffer;
		ConstantBuffer<EmitterCBuffer> emitter_cbuffer;
		ConstantBuffer<SortDispatchInfo> sort_dispatch_info_cbuffer;

		Buffer indirect_render_args_buffer;
		Buffer indirect_sort_args_buffer;
		std::unique_ptr<Buffer> index_buffer;

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