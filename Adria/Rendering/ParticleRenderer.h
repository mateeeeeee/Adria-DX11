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
#include "../Graphics/StructuredBuffer.h"
#include "../Graphics/AppendBuffer.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/GraphicsCoreDX11.h"

namespace adria
{
	//based on AMD GPU Particles Sample: https://github.com/GPUOpen-LibrariesAndSDKs/GPUParticles11

	class ParticleRenderer
	{
		static constexpr size_t MAX_PARTICLES = 400 * 1024;

		struct GPUParticleA
		{
			DirectX::XMFLOAT4	TintAndAlpha;	
			F32		Rotation;					
			U32		IsSleeping;					
		};
		struct GPUParticleB
		{
			DirectX::XMFLOAT3	Position;		
			F32		Mass;						

			DirectX::XMFLOAT3	Velocity;		
			F32		Lifespan;					

			F32		DistanceToEye;				
			F32		Age;						
			F32		StartSize;					
			F32		EndSize;					
		};
		struct EmitterCBuffer
		{
			DirectX::XMFLOAT4	EmitterPosition;
			DirectX::XMFLOAT4	EmitterVelocity;
			DirectX::XMFLOAT4	PositionVariance;

			I32	MaxParticlesThisFrame;
			F32	ParticleLifeSpan;
			F32	StartSize;
			F32	EndSize;
			
			F32	VelocityVariance;
			F32	Mass;
			F32	ElapsedTime;
			I32 Collisions;

			I32 CollisionThickness;
		};
		struct IndexBufferElement
		{
			F32	distance;	
			F32	index;		
		};
		struct ViewSpacePositionRadius
		{
			DirectX::XMFLOAT3 viewspace_position;
			F32 radius;
		};
		struct SortDispatchInfo
		{
			I32 x, y, z, w;
		};
	public:
		ParticleRenderer(GraphicsCoreDX11* gfx);

		void Update(F32 dt, Emitter& emitter_params);

		void Render(Emitter const& emitter_params,
					ID3D11ShaderResourceView* depth_srv, 
					ID3D11ShaderResourceView* particle_srv);

	private:
		GraphicsCoreDX11* gfx;

		Texture2D random_texture;
		AppendBuffer<U32> dead_list_buffer;
		StructuredBuffer<GPUParticleA> particle_bufferA;
		StructuredBuffer<GPUParticleB> particle_bufferB;
		StructuredBuffer<ViewSpacePositionRadius> view_space_positions_buffer;
		StructuredBuffer<IndexBufferElement> alive_index_buffer;

		ConstantBuffer<U32> dead_list_count_cbuffer;
		ConstantBuffer<U32> active_list_count_cbuffer;
		ConstantBuffer<EmitterCBuffer> emitter_cbuffer;
		ConstantBuffer<SortDispatchInfo> sort_dispatch_info_cbuffer;

		Microsoft::WRL::ComPtr<ID3D11Buffer> indirect_render_args_buffer;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> indirect_render_args_uav;
		Microsoft::WRL::ComPtr<ID3D11Buffer> indirect_sort_args_buffer;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> indirect_sort_args_uav;
		IndexBuffer index_buffer;

		std::unordered_map<EComputeShader, ComputeProgram>  particle_compute_programs;
		std::unordered_map<EShader, StandardProgram>  particle_render_programs;
	
private:
		void LoadShaders();
		void CreateIndirectArgsBuffers();
		void CreateIndexBuffer();
		void CreateRandomTexture();

		void InitializeDeadList();
		void ResetParticles();
		void Emit(Emitter const& emitter_params);
		void Simulate(ID3D11ShaderResourceView* depth_srv);
		void Rasterize(Emitter const& emitter_params, ID3D11ShaderResourceView* depth_srv, ID3D11ShaderResourceView* particle_srv);
		void Sort();
		
		bool SortInitial();
		bool SortIncremental(U32 presorted);
	};
}