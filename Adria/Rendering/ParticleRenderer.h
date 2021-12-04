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
#include "../Utilities/Random.h"

namespace adria
{
	//based on AMD GPU Particles Sample: https://github.com/GPUOpen-LibrariesAndSDKs/GPUParticles11

	class ParticleRenderer
	{
		static constexpr size_t MAX_PARTICLES = 400 * 1024;

		struct GPUParticleA
		{
			DirectX::XMFLOAT4	TintAndAlpha;	// The color and opacity
			f32		Rotation;					// The rotation angle
			u32		IsSleeping;					// Whether or not the particle is sleeping (ie, don't update position)
		};
		struct GPUParticleB
		{
			DirectX::XMFLOAT3	Position;		// World space position
			f32		Mass;						// Mass of particle

			DirectX::XMFLOAT3	Velocity;		// World space velocity
			f32		Lifespan;					// Lifespan of the particle.

			f32		DistanceToEye;				// The distance from the particle to the eye
			f32		Age;						// The current age counting down from lifespan to zero
			f32		StartSize;					// The size at spawn time
			f32		EndSize;					// The time at maximum age
		};
		struct EmitterCBuffer
		{
			DirectX::XMFLOAT4	EmitterPosition;
			DirectX::XMFLOAT4	EmitterVelocity;
			DirectX::XMFLOAT4	PositionVariance;

			i32	MaxParticlesThisFrame;
			f32	ParticleLifeSpan;
			f32	StartSize;
			f32	EndSize;
			
			f32	VelocityVariance;
			f32	Mass;
			f32	ElapsedTime;
			i32 Collisions;

			i32 CollisionThickness;
		};
		struct IndexBufferElement
		{
			f32	distance;	// distance squared from the particle to the camera
			f32	index;		// global index of the particle
		};
		struct ViewSpacePositionRadius
		{
			DirectX::XMFLOAT3 viewspace_position;
			f32 radius;
		};
		struct SortDispatchInfo
		{
			i32 x, y, z, w;
		};
	public:
		ParticleRenderer(GraphicsCoreDX11* gfx) : gfx{ gfx },
			dead_list_buffer(gfx->Device(), MAX_PARTICLES),
			particle_bufferA(gfx->Device(), MAX_PARTICLES),
			particle_bufferB(gfx->Device(), MAX_PARTICLES),
			view_space_positions_buffer(gfx->Device(), MAX_PARTICLES),
			dead_list_count_cbuffer(gfx->Device(), false),
			active_list_count_cbuffer(gfx->Device(), false),
			emitter_cbuffer(gfx->Device(), true),
			sort_dispatch_info_cbuffer(gfx->Device(), true),
			alive_index_buffer(gfx->Device(), MAX_PARTICLES, false, true)
		{
			LoadShaders();
			CreateRandomTexture();
			CreateIndexBuffer();
			CreateIndirectArgsBuffers();
		}

		void Update(f32 dt, Emitter& emitter_params)
		{
			emitter_params.elapsed_time += dt;
			if (emitter_params.particles_per_second > 0.0f)
			{
				emitter_params.accumulation += emitter_params.particles_per_second * dt;

				if (emitter_params.accumulation > 1.0f)
				{
					f64 integer_part = 0.0;
					f32 fraction = (f32)modf(emitter_params.accumulation, &integer_part);

					emitter_params.number_to_emit = (i32)integer_part;
					emitter_params.accumulation = fraction;
				}
			}
		}

		void Render(Emitter const& emitter_params,
					ID3D11ShaderResourceView* depth_srv, 
					ID3D11ShaderResourceView* particle_srv)
		{
			if (emitter_params.reset_emitter)
			{
				InitializeDeadList();
				ResetParticles();
				emitter_params.reset_emitter = false;
			}
			Emit(emitter_params);
			Simulate(depth_srv);
			if (emitter_params.sort)
			{
				Sort();
			}
			Rasterize(emitter_params, depth_srv, particle_srv);
		}

	private:
		GraphicsCoreDX11* gfx;

		Texture2D random_texture;
		AppendBuffer<u32> dead_list_buffer;
		StructuredBuffer<GPUParticleA> particle_bufferA;
		StructuredBuffer<GPUParticleB> particle_bufferB;
		StructuredBuffer<ViewSpacePositionRadius> view_space_positions_buffer;
		StructuredBuffer<IndexBufferElement> alive_index_buffer;

		ConstantBuffer<u32> dead_list_count_cbuffer;
		ConstantBuffer<u32> active_list_count_cbuffer;
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
		void LoadShaders()
		{
			ID3D11Device* device = gfx->Device();
			ShaderBlob cs_blob;

			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/InitDeadListCS.cso", cs_blob);
			particle_compute_programs[EComputeShader::ParticleInitDeadList].Create(device, cs_blob);

			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticleResetCS.cso", cs_blob);
			particle_compute_programs[EComputeShader::ParticleReset].Create(device, cs_blob);

			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticleEmitCS.cso", cs_blob);
			particle_compute_programs[EComputeShader::ParticleEmit].Create(device, cs_blob);

			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticleSimulateCS.cso", cs_blob);
			particle_compute_programs[EComputeShader::ParticleSimulate].Create(device, cs_blob);
			
			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/BitonicSortStepCS.cso", cs_blob);
			particle_compute_programs[EComputeShader::ParticleBitonicSortStep].Create(device, cs_blob);

			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/Sort512CS.cso", cs_blob);
			particle_compute_programs[EComputeShader::ParticleSort512].Create(device, cs_blob);

			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/SortInner512CS.cso", cs_blob);
			particle_compute_programs[EComputeShader::ParticleSortInner512].Create(device, cs_blob);

			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/InitSortDispatchArgsCS.cso", cs_blob);
			particle_compute_programs[EComputeShader::ParticleSortInitArgs].Create(device, cs_blob);

			ShaderBlob vs_blob, ps_blob;
			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticleVS.cso", vs_blob);
			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticlePS.cso", ps_blob);
			particle_render_programs[EShader::Particles].Create(device, vs_blob, ps_blob);
		}
		void CreateIndirectArgsBuffers()
		{
			ID3D11Device* device = gfx->Device();

			//rendering particles
			{
				D3D11_BUFFER_DESC desc{};
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
				desc.ByteWidth = 5 * sizeof(UINT);
				desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
				device->CreateBuffer(&desc, nullptr, &indirect_render_args_buffer);

				D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
				uav_desc.Format = DXGI_FORMAT_R32_UINT;
				uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
				uav_desc.Buffer.FirstElement = 0;
				uav_desc.Buffer.NumElements = 5;
				uav_desc.Buffer.Flags = 0;
				device->CreateUnorderedAccessView(indirect_render_args_buffer.Get(), &uav_desc, &indirect_render_args_uav);
			}

			//sorting particles
			{

				D3D11_BUFFER_DESC desc{};
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
				desc.ByteWidth = 4 * sizeof(UINT);
				desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
				device->CreateBuffer(&desc, nullptr, &indirect_sort_args_buffer);

				D3D11_UNORDERED_ACCESS_VIEW_DESC uav{};
				uav.Format = DXGI_FORMAT_R32_UINT;
				uav.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
				uav.Buffer.FirstElement = 0;
				uav.Buffer.NumElements = 4;
				uav.Buffer.Flags = 0;
				device->CreateUnorderedAccessView(indirect_sort_args_buffer.Get(), &uav, &indirect_sort_args_uav);
			}

		}
		void CreateIndexBuffer()
		{
			ID3D11Device* device = gfx->Device();

			std::vector<UINT> indices(MAX_PARTICLES * 6);
			UINT base = 0;
			size_t offset = 0;
			for (size_t i = 0; i < MAX_PARTICLES; i++)
			{
				indices[offset + 0] = base + 0;
				indices[offset + 1] = base + 1;
				indices[offset + 2] = base + 2;

				indices[offset + 3] = base + 2;
				indices[offset + 4] = base + 1;
				indices[offset + 5] = base + 3;

				base += 4;
				offset += 6;
			}

			index_buffer.Create(device, indices);
		}
		void CreateRandomTexture()
		{
			ID3D11Device* device = gfx->Device();

			texture2d_desc_t desc{};
			desc.width = 1024;
			desc.height = 1024;
			desc.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			desc.usage = D3D11_USAGE_IMMUTABLE;
			desc.bind_flags = D3D11_BIND_SHADER_RESOURCE;

			desc.srv_desc.format = desc.format;
			desc.srv_desc.most_detailed_mip = 0;
			desc.srv_desc.mip_levels = 1;

			std::vector<f32> random_texture_data;
			RealRandomGenerator rand_float{ 0.0f, 1.0f };
			for (u32 i = 0; i < desc.width * desc.height; i++)
			{
				random_texture_data.push_back(2.0f * rand_float() - 1.0f);
				random_texture_data.push_back(2.0f * rand_float() - 1.0f);
				random_texture_data.push_back(2.0f * rand_float() - 1.0f);
				random_texture_data.push_back(2.0f * rand_float() - 1.0f);
			}

			desc.init_data.data = (void*)random_texture_data.data();
			desc.init_data.pitch = desc.width * 4 * sizeof(f32);

			random_texture = Texture2D(device, desc);
		}

		void InitializeDeadList()
		{
			ID3D11DeviceContext* context = gfx->Context();

			u32 initial_count[] = { 0 };
			ID3D11UnorderedAccessView* dead_list_uavs[] = { dead_list_buffer.UAV() };
			context->CSSetUnorderedAccessViews(0, 1, dead_list_uavs, initial_count);

			particle_compute_programs[EComputeShader::ParticleInitDeadList].Bind(context);
			context->Dispatch((UINT)std::ceil(MAX_PARTICLES * 1.0f / 256), 1, 1);
			particle_compute_programs[EComputeShader::ParticleInitDeadList].Unbind(context);

			ZeroMemory(dead_list_uavs, sizeof(dead_list_uavs));
			context->CSSetUnorderedAccessViews(0, 1, dead_list_uavs, nullptr);
		}
		void ResetParticles()
		{
			ID3D11DeviceContext* context = gfx->Context();

			ID3D11UnorderedAccessView* uavs[] = { particle_bufferA.UAV(), particle_bufferB.UAV()};
			u32 initial_counts[] = { (u32)-1, (u32)-1 };
			context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, initial_counts);

			particle_compute_programs[EComputeShader::ParticleReset].Bind(context);
			context->Dispatch((UINT)std::ceil(MAX_PARTICLES * 1.0f / 256), 1, 1);
			particle_compute_programs[EComputeShader::ParticleReset].Unbind(context);

			ZeroMemory(uavs, sizeof(uavs));
			context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, nullptr);
		}
		void Emit(Emitter const& emitter_params)
		{
			ID3D11DeviceContext* context = gfx->Context();

			ID3DUserDefinedAnnotation* annotation = gfx->Annotation();
			annotation->BeginEvent(L"Particles Emit Pass");

			if (emitter_params.number_to_emit > 0)
			{
				particle_compute_programs[EComputeShader::ParticleEmit].Bind(context);

				ID3D11UnorderedAccessView* uavs[] = { particle_bufferA.UAV(), particle_bufferB.UAV(), dead_list_buffer.UAV() };
				u32 initial_counts[] = { (u32)-1, (u32)-1, (u32)-1 };
				context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, initial_counts);

				ID3D11ShaderResourceView* srvs[] = { random_texture.SRV() };
				context->CSSetShaderResources(0, ARRAYSIZE(srvs), srvs);

				EmitterCBuffer emitter_cbuffer_data{};
				emitter_cbuffer_data.ElapsedTime = emitter_params.elapsed_time;
				emitter_cbuffer_data.EmitterPosition = emitter_params.position;
				emitter_cbuffer_data.EmitterVelocity = emitter_params.velocity;
				emitter_cbuffer_data.StartSize = emitter_params.start_size;
				emitter_cbuffer_data.EndSize = emitter_params.end_size;
				emitter_cbuffer_data.Mass = emitter_params.mass;
				emitter_cbuffer_data.MaxParticlesThisFrame = emitter_params.number_to_emit;
				emitter_cbuffer_data.ParticleLifeSpan = emitter_params.particle_lifespan;
				emitter_cbuffer_data.PositionVariance = emitter_params.position_variance;
				emitter_cbuffer_data.VelocityVariance = emitter_params.velocity_variance;
				emitter_cbuffer_data.Collisions = emitter_params.collisions_enabled;
				emitter_cbuffer_data.CollisionThickness = emitter_params.collision_thickness;
				emitter_cbuffer.Update(context, emitter_cbuffer_data);

				dead_list_count_cbuffer.Bind(context, ShaderStage::CS, 11);
				emitter_cbuffer.Bind(context, ShaderStage::CS, 13);

				context->CopyStructureCount(dead_list_count_cbuffer.Buffer(), 0, dead_list_buffer.UAV());
				u32 thread_groups_x = (UINT)std::ceil(emitter_params.number_to_emit * 1.0f / 1024);
				context->Dispatch(thread_groups_x, 1, 1);

				ZeroMemory(srvs, sizeof(srvs));
				context->CSSetShaderResources(0, ARRAYSIZE(srvs), srvs);

				ZeroMemory(uavs, sizeof(uavs));
				context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, nullptr);

				particle_compute_programs[EComputeShader::ParticleEmit].Unbind(context);

			}

			annotation->EndEvent();
		}
		void Simulate(ID3D11ShaderResourceView* depth_srv)
		{
			ID3D11DeviceContext* context = gfx->Context();

			ID3DUserDefinedAnnotation* annotation = gfx->Annotation();
			annotation->BeginEvent(L"Particles Simulate Pass");

			ID3D11UnorderedAccessView* uavs[] = {
				particle_bufferA.UAV(), particle_bufferB.UAV(),
				dead_list_buffer.UAV(), alive_index_buffer.UAV(),
				view_space_positions_buffer.UAV(), indirect_render_args_uav.Get()};
			u32 initial_counts[] = { (u32)-1, (u32)-1, (u32)-1, 0, (u32)-1, (u32)-1 };
			context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, initial_counts);

			ID3D11ShaderResourceView* srvs[] = { depth_srv };
			context->CSSetShaderResources(0, ARRAYSIZE(srvs), srvs);

			particle_compute_programs[EComputeShader::ParticleSimulate].Bind(context);
			context->Dispatch((UINT)std::ceil(MAX_PARTICLES * 1.0f / 256 ), 1, 1);

			ZeroMemory(uavs, sizeof(uavs));
			context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, nullptr);

			ZeroMemory(srvs, sizeof(srvs));
			context->CSSetShaderResources(0, ARRAYSIZE(srvs), srvs);

			context->CopyStructureCount(active_list_count_cbuffer.Buffer(), 0, alive_index_buffer.UAV());
			annotation->EndEvent();
		}
		void Rasterize(Emitter const& emitter_params, ID3D11ShaderResourceView* depth_srv, ID3D11ShaderResourceView* particle_srv)
		{
			ID3D11DeviceContext* context = gfx->Context();

			ID3DUserDefinedAnnotation* annotation = gfx->Annotation();
			annotation->BeginEvent(L"Particles Rasterize Pass");

			active_list_count_cbuffer.Bind(context, ShaderStage::VS, 12);

			ID3D11Buffer* vb = nullptr;
			UINT stride = 0;
			UINT offset = 0;
			context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
			index_buffer.Bind(context);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			ID3D11ShaderResourceView* vs_srvs[] = { particle_bufferA.SRV(), view_space_positions_buffer.SRV(), alive_index_buffer.SRV() };
			ID3D11ShaderResourceView* ps_srvs[] = { particle_srv, depth_srv }; 
			context->VSSetShaderResources(0, ARRAYSIZE(vs_srvs), vs_srvs);
			context->PSSetShaderResources(0, ARRAYSIZE(ps_srvs), ps_srvs);

			particle_render_programs[EShader::Particles].Bind(context);
			context->DrawIndexedInstancedIndirect(indirect_render_args_buffer.Get(), 0);

			ZeroMemory(vs_srvs, sizeof(vs_srvs));
			context->VSSetShaderResources(0, ARRAYSIZE(vs_srvs), vs_srvs);
			ZeroMemory(ps_srvs, sizeof(ps_srvs));
			context->PSSetShaderResources(0, ARRAYSIZE(ps_srvs), ps_srvs);

			annotation->EndEvent();
		}
		void Sort()
		{
			ID3D11DeviceContext* context = gfx->Context();

			ID3DUserDefinedAnnotation* annotation = gfx->Annotation();
			annotation->BeginEvent(L"Particles Sort Pass");

			active_list_count_cbuffer.Bind(context, ShaderStage::CS, 11);
			sort_dispatch_info_cbuffer.Bind(context, ShaderStage::CS, 12);

			// Write the indirect args to a UAV
			context->CSSetUnorderedAccessViews(0, 1, indirect_sort_args_uav.GetAddressOf(), nullptr);
			particle_compute_programs[EComputeShader::ParticleSortInitArgs].Bind(context);
			context->Dispatch(1, 1, 1);

			ID3D11UnorderedAccessView* uav = alive_index_buffer.UAV();
			context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

			bool done = SortInitial();
			u32 presorted = 512;
			while (!done)
			{
				done = SortIncremental(presorted);
				presorted *= 2;
			}

			uav = nullptr;
			context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

			annotation->EndEvent();
		}
		
		bool SortInitial()
		{
			ID3D11DeviceContext* context = gfx->Context();

			bool done = true;
			UINT numThreadGroups = ((MAX_PARTICLES - 1) >> 9) + 1;
			if (numThreadGroups > 1) done = false;

			particle_compute_programs[EComputeShader::ParticleSort512].Bind(context);
			context->DispatchIndirect(indirect_sort_args_buffer.Get(), 0);
			return done;
		}
		bool SortIncremental(u32 presorted)
		{
			ID3D11DeviceContext* context = gfx->Context();

			bool done = true;
			particle_compute_programs[EComputeShader::ParticleBitonicSortStep].Bind(context);

			UINT num_thread_groups = 0;
			if (MAX_PARTICLES > presorted)
			{
				if (MAX_PARTICLES > presorted * 2) done = false;
				UINT pow2 = presorted;
				while (pow2 < MAX_PARTICLES) pow2 *= 2;
				num_thread_groups = pow2 >> 9;
			}

			u32 merge_size = presorted * 2;
			for (u32 merge_subsize = merge_size >> 1; merge_subsize > 256; merge_subsize = merge_subsize >> 1)
			{

				SortDispatchInfo sort_dispatch_info{};
				sort_dispatch_info.x = merge_subsize;
				if (merge_subsize == merge_size >> 1)
				{
					sort_dispatch_info.y = (2 * merge_subsize - 1);
					sort_dispatch_info.z = -1;
				}
				else
				{
					sort_dispatch_info.y = merge_subsize;
					sort_dispatch_info.z = 1;
				}
				sort_dispatch_info.w = 0;
				sort_dispatch_info_cbuffer.Update(context, sort_dispatch_info);
				context->Dispatch(num_thread_groups, 1, 1);
			}

			particle_compute_programs[EComputeShader::ParticleSortInner512].Bind(context);
			context->Dispatch(num_thread_groups, 1, 1);

			return done;
		}
	};
}