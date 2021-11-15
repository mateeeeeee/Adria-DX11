#pragma once
#include <unordered_map>
#include <DirectXMath.h>
#include "Enums.h"
#include "../tecs/registry.h"
#include "../Graphics/ShaderProgram.h"
#include "../Graphics/ConstantBuffer.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/StructuredBuffer.h"
#include "../Graphics/AppendBuffer.h"
#include "../Graphics/GraphicsCoreDX11.h"

namespace adria
{
	class ParticleSystem
	{
		static constexpr size_t MAX_PARTICLES = 400 * 1024;

		struct GPUParticleACBuffer
		{
			DirectX::XMFLOAT4	TintAndAlpha;			// The color and opacity

			DirectX::XMFLOAT2	VelocityXY;				// View space velocity XY used for streak extrusion
			float	EmitterNdotL;			// The lighting term for the while emitter
			u32		EmitterProperties;		// The index of the emitter in 0-15 bits, 16-23 for atlas index, 24th bit is whether or not the emitter supports velocity-based streaks

			float	Rotation;				// The rotation angle
			u32		IsSleeping;				// Whether or not the particle is sleeping (ie, don't update position)
			u32		CollisionCount;			// Keep track of how many times the particle has collided
			float	pads[1];
		};

		struct GPUParticleBCBuffer
		{
			DirectX::XMFLOAT3	Position;	// World space position
			float	Mass;					// Mass of particle

			DirectX::XMFLOAT3	Velocity;	// World space velocity
			float	Lifespan;				// Lifespan of the particle.

			float	DistanceToEye;			// The distance from the particle to the eye
			float	Age;					// The current age counting down from lifespan to zero
			float	StartSize;				// The size at spawn time
			float	EndSize;				// The time at maximum age
		};

	public:
		ParticleSystem(tecs::registry& reg, GraphicsCoreDX11* gfx) : reg{ reg }, gfx{ gfx },
			dead_list_buffer(gfx->Device(), MAX_PARTICLES),
			particle_bufferA(gfx->Device(), MAX_PARTICLES),
			particle_bufferB(gfx->Device(), MAX_PARTICLES)
		{
			LoadShaders();
		}


		void Render()
		{
			ID3D11DeviceContext* context = gfx->Context();
			if (reset)
			{
				InitializeDeadList();
				ResetParticles();
				reset = false;
			}

			Emit();

			Simulate();

			//context->CopyStructureCount(m_pActiveListConstantBuffer, 0, m_pAliveIndexBufferUAV);

		}

		void Reset()
		{
			reset = true;
		}

	private:
		tecs::registry& reg;
		GraphicsCoreDX11* gfx;
		AppendBuffer<u32> dead_list_buffer;
		StructuredBuffer<GPUParticleACBuffer> particle_bufferA;
		StructuredBuffer<GPUParticleBCBuffer> particle_bufferB;

		std::unordered_map<EComputeShader, ComputeProgram>  particle_compute_programs;
		bool reset = true;

	private:
		void LoadShaders()
		{
			ID3D11Device* device = gfx->Device();
			ShaderBlob cs_blob;

			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/InitDeadListCS.cso", cs_blob);
			particle_compute_programs[EComputeShader::ParticleInitDeadList].Create(device, cs_blob);

			ShaderUtility::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticleResetCS.cso", cs_blob);
			particle_compute_programs[EComputeShader::ParticleReset].Create(device, cs_blob);
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
			u32 initialCounts[] = { (u32)-1, (u32)-1 };
			context->CSSetUnorderedAccessViews(0, _countof(uavs), uavs, initialCounts);

			particle_compute_programs[EComputeShader::ParticleReset].Bind(context);
			context->Dispatch(std::ceil(MAX_PARTICLES * 1.0f / 256), 1, 1);
			particle_compute_programs[EComputeShader::ParticleReset].Unbind(context);
		}

		void Emit()
		{

		}

		void Simulate()
		{

		}
	};
}