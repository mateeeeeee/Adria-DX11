#include "Utilities/Random.h"
#include "Graphics/GfxScopedAnnotation.h"
#include "Graphics/GfxCommandContext.h"
#include "ParticleRenderer.h"
#include "ShaderManager.h"

namespace adria
{
	ParticleRenderer::ParticleRenderer(GfxDevice* gfx) : gfx{ gfx },
		dead_list_buffer(gfx, AppendBufferDesc(MAX_PARTICLES, sizeof(Uint32))),
		particle_bufferA(gfx, StructuredBufferDesc<GPUParticleA>(MAX_PARTICLES)),
		particle_bufferB(gfx, StructuredBufferDesc<GPUParticleB>(MAX_PARTICLES)),
		view_space_positions_buffer(gfx, StructuredBufferDesc<ViewSpacePositionRadius>(MAX_PARTICLES)),
		alive_index_buffer(gfx, StructuredBufferDesc<IndexBufferElement>(MAX_PARTICLES)),
		indirect_render_args_buffer(gfx, IndirectArgsBufferDesc(5 * sizeof(Uint32))),
		indirect_sort_args_buffer(gfx, IndirectArgsBufferDesc(4 * sizeof(Uint32))),
		dead_list_count_cbuffer(gfx, false),
		active_list_count_cbuffer(gfx, false),
		emitter_cbuffer(gfx, true),
		sort_dispatch_info_cbuffer(gfx, true)
	{
		CreateViews();
		CreateRandomTexture();
		CreateIndexBuffer();
	}

	void ParticleRenderer::Update(Float dt, Emitter& emitter_params)
	{
		emitter_params.elapsed_time += dt;
		if (emitter_params.particles_per_second > 0.0f)
		{
			emitter_params.accumulation += emitter_params.particles_per_second * dt;
			if (emitter_params.accumulation > 1.0f)
			{
				Float64 integer_part = 0.0;
				Float fraction = (Float)modf(emitter_params.accumulation, &integer_part);

				emitter_params.number_to_emit = (Sint32)integer_part;
				emitter_params.accumulation = fraction;
			}
		}
	}

	void ParticleRenderer::Render(Emitter const& emitter_params, GfxShaderResourceRO depth_srv, GfxShaderResourceRO particle_srv)
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

	void ParticleRenderer::CreateViews()
	{
		GfxBufferSubresourceDesc uav_desc{ .uav_flags = UAV_Append };
		dead_list_buffer.CreateUAV(&uav_desc);

		uav_desc.uav_flags = UAV_Counter;
		alive_index_buffer.CreateUAV(&uav_desc);
		alive_index_buffer.CreateSRV();

		particle_bufferA.CreateUAV();
		particle_bufferA.CreateSRV();
		particle_bufferB.CreateUAV();
		view_space_positions_buffer.CreateSRV();
		view_space_positions_buffer.CreateUAV();

		indirect_render_args_buffer.CreateUAV();
		indirect_sort_args_buffer.CreateUAV();
	}

	void ParticleRenderer::CreateIndexBuffer()
	{
		std::vector<Uint32> indices(MAX_PARTICLES * 6);
		Uint32 base = 0;
		Uint32 offset = 0;
		for (Uint32 i = 0; i < MAX_PARTICLES; i++)
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
		index_buffer = std::make_unique<GfxBuffer>(gfx, IndexBufferDesc(indices.size(), false), indices.data());
	}

	void ParticleRenderer::CreateRandomTexture()
	{
		GfxTextureDesc desc{};
		desc.width = 1024;
		desc.height = 1024;
		desc.format = GfxFormat::R32G32B32A32_FLOAT;
		desc.usage = GfxResourceUsage::Immutable;
		desc.bind_flags = GfxBindFlag::ShaderResource;

		std::vector<Float> random_texture_data;
		RealRandomGenerator rand_float{ 0.0f, 1.0f };
		for (Uint32 i = 0; i < desc.width * desc.height; i++)
		{
			random_texture_data.push_back(2.0f * rand_float() - 1.0f);
			random_texture_data.push_back(2.0f * rand_float() - 1.0f);
			random_texture_data.push_back(2.0f * rand_float() - 1.0f);
			random_texture_data.push_back(2.0f * rand_float() - 1.0f);
		}
		GfxTextureInitialData init_data{};
		init_data.pSysMem = (void*)random_texture_data.data();
		init_data.SysMemPitch = desc.width * 4 * sizeof(Float);

		random_texture = std::make_unique<GfxTexture>(gfx, desc, &init_data);
		random_texture->CreateSRV();
	}

	void ParticleRenderer::InitializeDeadList() 
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		
		Uint32 initial_count[] = { 0 };
		GfxShaderResourceRW dead_list_uavs[] = { dead_list_buffer.UAV() };
		command_context->SetShaderResourcesRW(0, dead_list_uavs, initial_count);

		ShaderManager::GetShaderProgram(ShaderProgram::ParticleInitDeadList)->Bind(command_context);
		command_context->Dispatch((Uint32)std::ceil(MAX_PARTICLES * 1.0f / 256), 1, 1);
		ShaderManager::GetShaderProgram(ShaderProgram::ParticleInitDeadList)->Unbind(command_context);

		command_context->UnsetShaderResourcesRW(0, ARRAYSIZE(dead_list_uavs));
	}

	void ParticleRenderer::ResetParticles()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		
		GfxShaderResourceRW uavs[] = { particle_bufferA.UAV(), particle_bufferB.UAV() };
		Uint32 initial_counts[] = { (Uint32)-1, (Uint32)-1 };
		command_context->SetShaderResourcesRW(0, uavs, initial_counts);

		ShaderManager::GetShaderProgram(ShaderProgram::ParticleReset)->Bind(command_context);
		command_context->Dispatch((Uint32)std::ceil(MAX_PARTICLES * 1.0f / 256), 1, 1);
		ShaderManager::GetShaderProgram(ShaderProgram::ParticleReset)->Unbind(command_context);

		command_context->UnsetShaderResourcesRW(0, ARRAYSIZE(uavs));
	}

	void ParticleRenderer::Emit(Emitter const& emitter_params)
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxScopedAnnotation(command_context,"Particles Emit Pass");

		if (emitter_params.number_to_emit > 0)
		{
			ShaderManager::GetShaderProgram(ShaderProgram::ParticleEmit)->Bind(command_context);

			GfxShaderResourceRW uavs[] = { particle_bufferA.UAV(), particle_bufferB.UAV(), dead_list_buffer.UAV() };
			Uint32 initial_counts[] = { (Uint32)-1, (Uint32)-1, (Uint32)-1 };
			command_context->SetShaderResourcesRW(0, uavs, initial_counts);

			GfxShaderResourceRO srvs[] = { random_texture->SRV() };
			command_context->SetShaderResourcesRO(GfxShaderStage::CS, 0, srvs);

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
			emitter_cbuffer.Update(command_context, emitter_cbuffer_data);

			dead_list_count_cbuffer.Bind(command_context, GfxShaderStage::CS, 11);
			emitter_cbuffer.Bind(command_context, GfxShaderStage::CS, 13);

			command_context->CopyStructureCount(dead_list_count_cbuffer.Buffer(), 0, dead_list_buffer.UAV());
			Uint32 thread_groups_x = (Uint32)std::ceil(emitter_params.number_to_emit * 1.0f / 1024);
			command_context->Dispatch(thread_groups_x, 1, 1);

			command_context->UnsetShaderResourcesRO(GfxShaderStage::CS, 0, ARRAYSIZE(srvs));
			command_context->UnsetShaderResourcesRW(0, ARRAYSIZE(uavs));

			ShaderManager::GetShaderProgram(ShaderProgram::ParticleEmit)->Unbind(command_context);
		}
	}

	void ParticleRenderer::Simulate(ID3D11ShaderResourceView* depth_srv)
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxScopedAnnotation(command_context, "Particles Simulate Pass");

		GfxShaderResourceRW uavs[] = {
			particle_bufferA.UAV(), particle_bufferB.UAV(),
			dead_list_buffer.UAV(), alive_index_buffer.UAV(),
			view_space_positions_buffer.UAV(), indirect_render_args_buffer.UAV() };
		Uint32 initial_counts[] = { (Uint32)-1, (Uint32)-1, (Uint32)-1, 0, (Uint32)-1, (Uint32)-1 };
		command_context->SetShaderResourcesRW(0, uavs, initial_counts);

		GfxShaderResourceRO srvs[] = { depth_srv };
		command_context->SetShaderResourcesRO(GfxShaderStage::CS, 0, srvs);

		ShaderManager::GetShaderProgram(ShaderProgram::ParticleSimulate)->Bind(command_context);
		command_context->Dispatch((Uint32)std::ceil(MAX_PARTICLES * 1.0f / 256), 1, 1);

		command_context->UnsetShaderResourcesRO(GfxShaderStage::CS, 0, ARRAYSIZE(srvs));
		command_context->UnsetShaderResourcesRW(0, ARRAYSIZE(uavs));

		command_context->CopyStructureCount(active_list_count_cbuffer.Buffer(), 0, alive_index_buffer.UAV());
	}

	void ParticleRenderer::Rasterize(Emitter const& emitter_params, GfxShaderResourceRO depth_srv, GfxShaderResourceRO particle_srv)
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxScopedAnnotation(command_context, "Particles Rasterize Pass");

		active_list_count_cbuffer.Bind(command_context, GfxShaderStage::VS, 12);

		command_context->SetVertexBuffer(nullptr);
		command_context->SetIndexBuffer(index_buffer.get());
		command_context->SetTopology(GfxPrimitiveTopology::TriangleList);
		
		GfxShaderResourceRO vs_srvs[] = { particle_bufferA.SRV(), view_space_positions_buffer.SRV(), alive_index_buffer.SRV() };
		GfxShaderResourceRO ps_srvs[] = { particle_srv, depth_srv };

		command_context->SetShaderResourcesRO(GfxShaderStage::VS, 0, vs_srvs);
		command_context->SetShaderResourcesRO(GfxShaderStage::PS, 0, ps_srvs);

		ShaderManager::GetShaderProgram(ShaderProgram::Particles)->Bind(command_context);
		command_context->DrawIndexedIndirect(indirect_render_args_buffer, 0);
		
		command_context->UnsetShaderResourcesRO(GfxShaderStage::VS, 0, ARRAYSIZE(vs_srvs));
		command_context->UnsetShaderResourcesRO(GfxShaderStage::PS, 0, ARRAYSIZE(ps_srvs));
	}

	void ParticleRenderer::Sort()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		AdriaGfxScopedAnnotation(command_context, "Particles Sort Pass");

		active_list_count_cbuffer.Bind(command_context, GfxShaderStage::CS, 11);
		sort_dispatch_info_cbuffer.Bind(command_context, GfxShaderStage::CS, 12);

		GfxShaderResourceRW indirect_sort_args_uav = indirect_sort_args_buffer.UAV();
		command_context->SetShaderResourceRW(0, indirect_sort_args_uav);
		ShaderManager::GetShaderProgram(ShaderProgram::ParticleSortInitArgs)->Bind(command_context);
		command_context->Dispatch(1, 1, 1);

		GfxShaderResourceRW uav = alive_index_buffer.UAV();
		command_context->SetShaderResourceRW(0, uav);

		Bool done = SortInitial();
		Uint32 presorted = 512;
		while (!done)
		{
			done = SortIncremental(presorted);
			presorted *= 2;
		}

		uav = nullptr;
		command_context->SetShaderResourceRW(0, nullptr);

	}

	Bool ParticleRenderer::SortInitial()
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		Bool done = true;
		Uint32 numThreadGroups = ((MAX_PARTICLES - 1) >> 9) + 1;
		if (numThreadGroups > 1) done = false;
		ShaderManager::GetShaderProgram(ShaderProgram::ParticleSort512)->Bind(command_context);
		command_context->DispatchIndirect(indirect_sort_args_buffer, 0);
		return done;
	}

	Bool ParticleRenderer::SortIncremental(Uint32 presorted)
	{
		GfxCommandContext* command_context = gfx->GetCommandContext();
		
		Bool done = true;
		ShaderManager::GetShaderProgram(ShaderProgram::ParticleBitonicSortStep)->Bind(command_context);

		Uint32 num_thread_groups = 0;
		if (MAX_PARTICLES > presorted)
		{
			if (MAX_PARTICLES > presorted * 2) done = false;
			Uint32 pow2 = presorted;
			while (pow2 < MAX_PARTICLES) pow2 *= 2;
			num_thread_groups = pow2 >> 9;
		}

		Uint32 merge_size = presorted * 2;
		for (Uint32 merge_subsize = merge_size >> 1; merge_subsize > 256; merge_subsize = merge_subsize >> 1)
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
			sort_dispatch_info_cbuffer.Update(command_context, sort_dispatch_info);
			command_context->Dispatch(num_thread_groups, 1, 1);
		}
		ShaderManager::GetShaderProgram(ShaderProgram::ParticleSortInner512)->Bind(command_context);
		command_context->Dispatch(num_thread_groups, 1, 1);

		return done;
	}

}

