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

	class ParticleSystem
	{
		static constexpr size_t MAX_PARTICLES = 400 * 1024;

		struct GPUParticleA
		{
			DirectX::XMFLOAT4	TintAndAlpha;  // The color and opacity
			f32		Rotation;				// The rotation angle
			u32		IsSleeping;				// Whether or not the particle is sleeping (ie, don't update position)
		};
		struct GPUParticleB
		{
			DirectX::XMFLOAT3	Position;	// World space position
			f32		Mass;					// Mass of particle

			DirectX::XMFLOAT3	Velocity;	// World space velocity
			f32		Lifespan;				// Lifespan of the particle.

			f32		DistanceToEye;			// The distance from the particle to the eye
			f32		Age;					// The current age counting down from lifespan to zero
			f32		StartSize;				// The size at spawn time
			f32		EndSize;				// The time at maximum age
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
	public:
		ParticleSystem(tecs::registry& reg, GraphicsCoreDX11* gfx) : reg{ reg }, gfx{ gfx },
			dead_list_buffer(gfx->Device(), MAX_PARTICLES),
			particle_bufferA(gfx->Device(), MAX_PARTICLES),
			particle_bufferB(gfx->Device(), MAX_PARTICLES),
			view_space_positions_buffer(gfx->Device(), MAX_PARTICLES),
			dead_list_count_cbuffer(gfx->Device(), false),
			active_list_count_cbuffer(gfx->Device(), false),
			emitter_cbuffer(gfx->Device(), true),
			alive_index_buffer(gfx->Device(), MAX_PARTICLES, false, true)
		{
			LoadShaders();
			CreateRandomTexture();
			CreateIndexBuffer();
			CreateIndirectArgsBuffer();
		}

		void Update(f32 dt)
		{
			elapsed_time += dt;

			auto emitters = reg.view<EmitterComponent>();
			for (auto emitter : emitters)
			{
				auto& emitter_params = emitters.get(emitter);

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
		}


		void Render()
		{
			if (reset)
			{
				InitializeDeadList();
				ResetParticles();
				reset = false;
			}

			Emit();

			Simulate();

			Rasterize();
		}

		void Reset()
		{
			reset = true;
		}

	private:
		tecs::registry& reg;
		GraphicsCoreDX11* gfx;

		bool reset = true;
		f32 elapsed_time = 0.0f;

		Texture2D random_texture;
		AppendBuffer<u32> dead_list_buffer;
		StructuredBuffer<GPUParticleA> particle_bufferA;
		StructuredBuffer<GPUParticleB> particle_bufferB;
		StructuredBuffer<ViewSpacePositionRadius> view_space_positions_buffer;
		StructuredBuffer<IndexBufferElement> alive_index_buffer;

		ConstantBuffer<u32> dead_list_count_cbuffer;
		ConstantBuffer<u32> active_list_count_cbuffer;
		ConstantBuffer<EmitterCBuffer> emitter_cbuffer;

		Microsoft::WRL::ComPtr<ID3D11Buffer> indirect_args_buffer;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> indirect_args_uav;
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

			ShaderBlob vs_blob, ps_blob;
			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticleVS.cso", vs_blob);
			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticlePS.cso", ps_blob);
			particle_render_programs[EShader::Particles].Create(device, vs_blob, ps_blob);
		}

		void CreateIndirectArgsBuffer()
		{
			ID3D11Device* device = gfx->Device();

			// Create the buffer to store the indirect args for the DrawInstancedIndirect call
			D3D11_BUFFER_DESC desc{};
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
			desc.ByteWidth = 5 * sizeof(UINT);
			desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
			device->CreateBuffer(&desc, nullptr, &indirect_args_buffer);

			D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
			uav_desc.Format = DXGI_FORMAT_R32_UINT;
			uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.FirstElement = 0;
			uav_desc.Buffer.NumElements = 5;
			uav_desc.Buffer.Flags = 0;
			device->CreateUnorderedAccessView(indirect_args_buffer.Get(), &uav_desc, &indirect_args_uav);
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
			ID3D11UnorderedAccessView* dead_list_uav = dead_list_buffer.UAV();
			context->CSSetUnorderedAccessViews(0, 1, &dead_list_uav, initial_count);

			particle_compute_programs[EComputeShader::ParticleInitDeadList].Bind(context);
			context->Dispatch(std::ceil(MAX_PARTICLES * 1.0f / 256), 1, 1);
			particle_compute_programs[EComputeShader::ParticleInitDeadList].Unbind(context);
		}

		void ResetParticles()
		{
			ID3D11DeviceContext* context = gfx->Context();

			ID3D11UnorderedAccessView* uavs[] = { particle_bufferA.UAV(), particle_bufferB.UAV()};
			u32 initial_counts[] = { (u32)-1, (u32)-1 };
			context->CSSetUnorderedAccessViews(0, _countof(uavs), uavs, initial_counts);

			particle_compute_programs[EComputeShader::ParticleReset].Bind(context);
			context->Dispatch(std::ceil(MAX_PARTICLES * 1.0f / 256), 1, 1);
			particle_compute_programs[EComputeShader::ParticleReset].Unbind(context);
		}

		void Emit()
		{
			ID3D11DeviceContext* context = gfx->Context();

			ID3D11UnorderedAccessView* uavs[] = { particle_bufferA.UAV(), particle_bufferB.UAV(), dead_list_buffer.UAV()};
			u32 initial_counts[] = { (u32)-1, (u32)-1, (u32)-1 };
			context->CSSetUnorderedAccessViews(0, _countof(uavs), uavs, initial_counts);

			emitter_cbuffer.Bind(context, ShaderStage::CS, 20);
			dead_list_count_cbuffer.Bind(context, ShaderStage::CS, 21);

			ID3D11ShaderResourceView* srvs[] = { random_texture.SRV() };
			context->CSSetShaderResources(0, _countof(srvs), srvs);

			auto emitters = reg.view<EmitterComponent>();
			for (auto emitter : emitters)
			{
				auto const& emitter_params = emitters.get(emitter);

				if (emitter_params.number_to_emit > 0)
				{
					EmitterCBuffer emitter_cbuffer_data{};
					emitter_cbuffer_data.ElapsedTime = elapsed_time;
					emitter_cbuffer_data.EmitterPosition = emitter_params.position;
					emitter_cbuffer_data.EmitterVelocity = emitter_params.velocity;
					emitter_cbuffer_data.StartSize = emitter_params.start_size;
					emitter_cbuffer_data.EndSize = emitter_params.end_size;
					emitter_cbuffer_data.Mass = emitter_params.mass;
					emitter_cbuffer_data.MaxParticlesThisFrame = emitter_params.number_to_emit;
					emitter_cbuffer_data.ParticleLifeSpan = emitter_params.particle_life_span;
					emitter_cbuffer_data.PositionVariance = emitter_params.position_variance;
					emitter_cbuffer_data.VelocityVariance = emitter_params.velocity_variance;
					emitter_cbuffer.Update(context, emitter_cbuffer_data);

					context->CopyStructureCount(dead_list_count_cbuffer.Buffer(), 0, dead_list_buffer.UAV());
					u32 thread_groups_x = std::ceil(emitter_params.number_to_emit / 1024);
					context->Dispatch(thread_groups_x, 1, 1);
				}
			}
		}

		void Simulate()
		{
			ID3D11DeviceContext* context = gfx->Context();

			ID3D11UnorderedAccessView* uavs[] = {
				particle_bufferA.UAV(), particle_bufferB.UAV(),
				dead_list_buffer.UAV(), alive_index_buffer.UAV(),
				view_space_positions_buffer.UAV(), indirect_args_uav.Get()};
			u32 initial_counts[] = { (u32)-1, (u32)-1, (u32)-1, 0, (u32)-1, (u32)-1 };
			context->CSSetUnorderedAccessViews(0, _countof(uavs), uavs, initial_counts);
			particle_compute_programs[EComputeShader::ParticleSimulate].Bind(context);
			context->Dispatch(std::ceil(MAX_PARTICLES / 256 ), 1, 1);
		}

		void Rasterize()
		{
			ID3D11DeviceContext* context = gfx->Context();

			context->CopyStructureCount(active_list_count_cbuffer.Buffer(), 0, alive_index_buffer.UAV());

			particle_render_programs[EShader::Particles].Bind(context);

			ID3D11ShaderResourceView* vs_srv[] = { particle_bufferA.SRV(), view_space_positions_buffer.SRV(), alive_index_buffer.SRV()};
			ID3D11ShaderResourceView* ps_srv[] = { nullptr }; //depth srv

			ID3D11Buffer* vb = nullptr;
			UINT stride = 0;
			UINT offset = 0;
			context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

			active_list_count_cbuffer.Bind(context, ShaderStage::VS, 12);

			index_buffer.Bind(context);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			context->VSSetShaderResources(0, _countof(vs_srv), vs_srv);
			context->PSSetShaderResources(1, _countof(ps_srv), ps_srv);

			context->DrawIndexedInstancedIndirect(indirect_args_buffer.Get(), 0);
		}
	};
}