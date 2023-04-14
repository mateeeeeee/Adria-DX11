#include "Utilities/Random.h"
#include "Graphics/GfxScopedAnnotation.h"
#include "ParticleRenderer.h"
#include "ShaderManager.h"

namespace adria
{
	ParticleRenderer::ParticleRenderer(GfxDevice* gfx) : gfx{ gfx },
		dead_list_buffer(gfx, AppendBufferDesc(MAX_PARTICLES, sizeof(uint32))),
		particle_bufferA(gfx, StructuredBufferDesc<GPUParticleA>(MAX_PARTICLES)),
		particle_bufferB(gfx, StructuredBufferDesc<GPUParticleB>(MAX_PARTICLES)),
		view_space_positions_buffer(gfx, StructuredBufferDesc<ViewSpacePositionRadius>(MAX_PARTICLES)),
		alive_index_buffer(gfx, StructuredBufferDesc<IndexBufferElement>(MAX_PARTICLES)),
		indirect_render_args_buffer(gfx, IndirectArgsBufferDesc(5 * sizeof(UINT))),
		indirect_sort_args_buffer(gfx, IndirectArgsBufferDesc(4 * sizeof(UINT))),
		dead_list_count_cbuffer(gfx->Device(), false),
		active_list_count_cbuffer(gfx->Device(), false),
		emitter_cbuffer(gfx->Device(), true),
		sort_dispatch_info_cbuffer(gfx->Device(), true)
	{
		CreateViews();
		CreateRandomTexture();
		CreateIndexBuffer();
	}

	void ParticleRenderer::Update(float32 dt, Emitter& emitter_params)
	{
		emitter_params.elapsed_time += dt;
		if (emitter_params.particles_per_second > 0.0f)
		{
			emitter_params.accumulation += emitter_params.particles_per_second * dt;
			if (emitter_params.accumulation > 1.0f)
			{
				float64 integer_part = 0.0;
				float32 fraction = (float32)modf(emitter_params.accumulation, &integer_part);

				emitter_params.number_to_emit = (int32)integer_part;
				emitter_params.accumulation = fraction;
			}
		}
	}

	void ParticleRenderer::Render(Emitter const& emitter_params, ID3D11ShaderResourceView* depth_srv, ID3D11ShaderResourceView* particle_srv)
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

		std::vector<float32> random_texture_data;
		RealRandomGenerator rand_float{ 0.0f, 1.0f };
		for (uint32 i = 0; i < desc.width * desc.height; i++)
		{
			random_texture_data.push_back(2.0f * rand_float() - 1.0f);
			random_texture_data.push_back(2.0f * rand_float() - 1.0f);
			random_texture_data.push_back(2.0f * rand_float() - 1.0f);
			random_texture_data.push_back(2.0f * rand_float() - 1.0f);
		}
		GfxTextureInitialData init_data{};
		init_data.pSysMem = (void*)random_texture_data.data();
		init_data.SysMemPitch = desc.width * 4 * sizeof(float32);

		random_texture = std::make_unique<GfxTexture>(gfx, desc, &init_data);
		random_texture->CreateSRV();
	}

	void ParticleRenderer::InitializeDeadList() 
	{
		ID3D11DeviceContext* context = gfx->Context();

		uint32 initial_count[] = { 0 };
		ID3D11UnorderedAccessView* dead_list_uavs[] = { dead_list_buffer.UAV() };
		context->CSSetUnorderedAccessViews(0, 1, dead_list_uavs, initial_count);

		ShaderManager::GetShaderProgram(EShaderProgram::ParticleInitDeadList)->Bind(context);
		context->Dispatch((UINT)std::ceil(MAX_PARTICLES * 1.0f / 256), 1, 1);
		ShaderManager::GetShaderProgram(EShaderProgram::ParticleInitDeadList)->Unbind(context);

		ZeroMemory(dead_list_uavs, sizeof(dead_list_uavs));
		context->CSSetUnorderedAccessViews(0, 1, dead_list_uavs, nullptr);
	}

	void ParticleRenderer::ResetParticles()
	{
		ID3D11DeviceContext* context = gfx->Context();

		ID3D11UnorderedAccessView* uavs[] = { particle_bufferA.UAV(), particle_bufferB.UAV() };
		uint32 initial_counts[] = { (uint32)-1, (uint32)-1 };
		context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, initial_counts);

		ShaderManager::GetShaderProgram(EShaderProgram::ParticleReset)->Bind(context);
		context->Dispatch((UINT)std::ceil(MAX_PARTICLES * 1.0f / 256), 1, 1);
		ShaderManager::GetShaderProgram(EShaderProgram::ParticleReset)->Unbind(context);

		ZeroMemory(uavs, sizeof(uavs));
		context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, nullptr);
	}

	void ParticleRenderer::Emit(Emitter const& emitter_params)
	{
		ID3D11DeviceContext* context = gfx->Context();
		SCOPED_ANNOTATION(gfx->Annotation(), L"Particles Emit Pass");

		if (emitter_params.number_to_emit > 0)
		{
			ShaderManager::GetShaderProgram(EShaderProgram::ParticleEmit)->Bind(context);

			ID3D11UnorderedAccessView* uavs[] = { particle_bufferA.UAV(), particle_bufferB.UAV(), dead_list_buffer.UAV() };
			uint32 initial_counts[] = { (uint32)-1, (uint32)-1, (uint32)-1 };
			context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, initial_counts);

			ID3D11ShaderResourceView* srvs[] = { random_texture->SRV() };
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

			dead_list_count_cbuffer.Bind(context, GfxShaderStage::CS, 11);
			emitter_cbuffer.Bind(context, GfxShaderStage::CS, 13);

			context->CopyStructureCount(dead_list_count_cbuffer.Buffer(), 0, dead_list_buffer.UAV());
			uint32 thread_groups_x = (UINT)std::ceil(emitter_params.number_to_emit * 1.0f / 1024);
			context->Dispatch(thread_groups_x, 1, 1);

			ZeroMemory(srvs, sizeof(srvs));
			context->CSSetShaderResources(0, ARRAYSIZE(srvs), srvs);

			ZeroMemory(uavs, sizeof(uavs));
			context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, nullptr);

			ShaderManager::GetShaderProgram(EShaderProgram::ParticleEmit)->Unbind(context);
		}
	}

	void ParticleRenderer::Simulate(ID3D11ShaderResourceView* depth_srv)
	{
		ID3D11DeviceContext* context = gfx->Context();
		SCOPED_ANNOTATION(gfx->Annotation(), L"Particles Simulate Pass");

		ID3D11UnorderedAccessView* uavs[] = {
			particle_bufferA.UAV(), particle_bufferB.UAV(),
			dead_list_buffer.UAV(), alive_index_buffer.UAV(),
			view_space_positions_buffer.UAV(), indirect_render_args_buffer.UAV() };
		uint32 initial_counts[] = { (uint32)-1, (uint32)-1, (uint32)-1, 0, (uint32)-1, (uint32)-1 };
		context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, initial_counts);

		ID3D11ShaderResourceView* srvs[] = { depth_srv };
		context->CSSetShaderResources(0, ARRAYSIZE(srvs), srvs);

		ShaderManager::GetShaderProgram(EShaderProgram::ParticleSimulate)->Bind(context); 
		context->Dispatch((UINT)std::ceil(MAX_PARTICLES * 1.0f / 256), 1, 1);

		ZeroMemory(uavs, sizeof(uavs));
		context->CSSetUnorderedAccessViews(0, ARRAYSIZE(uavs), uavs, nullptr);

		ZeroMemory(srvs, sizeof(srvs));
		context->CSSetShaderResources(0, ARRAYSIZE(srvs), srvs);

		context->CopyStructureCount(active_list_count_cbuffer.Buffer(), 0, alive_index_buffer.UAV());
	}

	void ParticleRenderer::Rasterize(Emitter const& emitter_params, ID3D11ShaderResourceView* depth_srv, ID3D11ShaderResourceView* particle_srv)
	{
		ID3D11DeviceContext* context = gfx->Context();
		SCOPED_ANNOTATION(gfx->Annotation(), L"Particles Rasterize Pass");

		active_list_count_cbuffer.Bind(context, GfxShaderStage::VS, 12);

		BindNullVertexBuffer(context);
		BindIndexBuffer(context, index_buffer.get());
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ID3D11ShaderResourceView* vs_srvs[] = { particle_bufferA.SRV(), view_space_positions_buffer.SRV(), alive_index_buffer.SRV() };
		ID3D11ShaderResourceView* ps_srvs[] = { particle_srv, depth_srv };
		context->VSSetShaderResources(0, ARRAYSIZE(vs_srvs), vs_srvs);
		context->PSSetShaderResources(0, ARRAYSIZE(ps_srvs), ps_srvs);

		ShaderManager::GetShaderProgram(EShaderProgram::Particles)->Bind(context);
		context->DrawIndexedInstancedIndirect(indirect_render_args_buffer.GetNative(), 0);

		ZeroMemory(vs_srvs, sizeof(vs_srvs));
		context->VSSetShaderResources(0, ARRAYSIZE(vs_srvs), vs_srvs);
		ZeroMemory(ps_srvs, sizeof(ps_srvs));
		context->PSSetShaderResources(0, ARRAYSIZE(ps_srvs), ps_srvs);
	}

	void ParticleRenderer::Sort()
	{
		ID3D11DeviceContext* context = gfx->Context();
		SCOPED_ANNOTATION(gfx->Annotation(), L"Particles Sort Pass");

		active_list_count_cbuffer.Bind(context, GfxShaderStage::CS, 11);
		sort_dispatch_info_cbuffer.Bind(context, GfxShaderStage::CS, 12);

		// Write the indirect args to a UAV
		ID3D11UnorderedAccessView* indirect_sort_args_uav = indirect_sort_args_buffer.UAV();
		context->CSSetUnorderedAccessViews(0, 1, &indirect_sort_args_uav, nullptr);
		ShaderManager::GetShaderProgram(EShaderProgram::ParticleSortInitArgs)->Bind(context);
		context->Dispatch(1, 1, 1);

		ID3D11UnorderedAccessView* uav = alive_index_buffer.UAV();
		context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

		bool done = SortInitial();
		uint32 presorted = 512;
		while (!done)
		{
			done = SortIncremental(presorted);
			presorted *= 2;
		}

		uav = nullptr;
		context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
	}

	bool ParticleRenderer::SortInitial()
	{
		ID3D11DeviceContext* context = gfx->Context();
		bool done = true;
		UINT numThreadGroups = ((MAX_PARTICLES - 1) >> 9) + 1;
		if (numThreadGroups > 1) done = false;
		ShaderManager::GetShaderProgram(EShaderProgram::ParticleSort512)->Bind(context);
		context->DispatchIndirect(indirect_sort_args_buffer.GetNative(), 0);
		return done;
	}

	bool ParticleRenderer::SortIncremental(uint32 presorted)
	{
		ID3D11DeviceContext* context = gfx->Context();

		bool done = true;
		ShaderManager::GetShaderProgram(EShaderProgram::ParticleBitonicSortStep)->Bind(context);

		UINT num_thread_groups = 0;
		if (MAX_PARTICLES > presorted)
		{
			if (MAX_PARTICLES > presorted * 2) done = false;
			UINT pow2 = presorted;
			while (pow2 < MAX_PARTICLES) pow2 *= 2;
			num_thread_groups = pow2 >> 9;
		}

		uint32 merge_size = presorted * 2;
		for (uint32 merge_subsize = merge_size >> 1; merge_subsize > 256; merge_subsize = merge_subsize >> 1)
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
		ShaderManager::GetShaderProgram(EShaderProgram::ParticleSortInner512)->Bind(context);
		context->Dispatch(num_thread_groups, 1, 1);

		return done;
	}

}

