#include "../Tasks/TaskSystem.h"
#include "Engine.h"
#include "Window.h"
#include "Events.h"
#include "../Math/Constants.h"
#include "../Logging/Logger.h"
#include "../Graphics/GraphicsCoreDX11.h"
#include "../Rendering/Renderer.h"
#include "../Editor/GUI.h"
#include "../Rendering/EntityLoader.h"
#include "../Utilities/Random.h"
#include "../Utilities/Timer.h"
#include "../Audio/AudioSystem.h"

namespace adria
{
	using namespace tecs;

	Engine::Engine(engine_init_t const& init)  : vsync{ init.vsync }, event_queue {}, input{ event_queue }, camera_manager{ input }
	{
		TaskSystem::Initialize();

		gfx = std::make_unique<GraphicsCoreDX11>(Window::Handle());
		renderer = std::make_unique<Renderer>(reg, gfx.get(), Window::Width(), Window::Height());
		entity_loader = std::make_unique<EntityLoader>(reg, gfx->Device(), renderer->GetTextureManager());
		
		camera_desc_t camera_desc{};
		camera_desc.aspect_ratio = static_cast<f32>(Window::Width()) / Window::Height();
		camera_desc.near_plane = 1.0f;
		camera_desc.far_plane = 3000.0f;
		camera_desc.fov = pi_div_4<f32>;
		camera_desc.position_x = 0.0f;
		camera_desc.position_y = 25.0f;
		camera_desc.position_z = 0.0f;
		camera_manager.AddCamera(camera_desc);
		InitializeScene(init.load_default_scene);

		event_queue.Subscribe<ResizeEvent>([this](ResizeEvent const& e) 
			{
				camera_manager.OnResize(e.width, e.height);
				gfx->ResizeBackbuffer(e.width, e.height);
				renderer->OnResize(e.width, e.height);
			});
		event_queue.Subscribe<ScrollEvent>([this](ScrollEvent const& e)
			{
				camera_manager.OnScroll(e.scroll);
			});
	}

	Engine::~Engine()
	{
		TaskSystem::Destroy();
	}

	void Engine::HandleWindowMessage(window_message_t const& msg_data)
	{
		input.HandleWindowMessage(msg_data);
	}

	void Engine::Run(RendererSettings const& settings, bool offscreen)
	{
		
		static EngineTimer timer;

		f32 const dt = timer.MarkInSeconds();

		if (Window::IsActive())
		{
			input.NewFrame();
			event_queue.ProcessEvents();

			Update(dt);
			Render(settings, offscreen);
		}
		else
		{
			input.NewFrame();
			event_queue.ProcessEvents();
		}

	}

	void Engine::Update(f32 dt)
	{
		camera_manager.Update(dt);
		auto& camera = camera_manager.GetActiveCamera();
		renderer->NewFrame(&camera);
		renderer->Update(dt);
	}

	void Engine::Render(RendererSettings const& settings, bool offscreen)
	{
		renderer->Render(settings);

		if (offscreen) renderer->ResolveToOffscreenFramebuffer();
		else
		{
			gfx->ClearBackbuffer();
			renderer->ResolveToBackbuffer();
		}
	}

	void Engine::Present()
	{
		gfx->SwapBuffers(vsync);
	}

	void Engine::InitializeScene(bool load_default_scene) 
	{

		skybox_parameters_t skybox_params{};
		skybox_params.cubemap = L"Resources/Textures/Skybox/sunsetcube1024.dds"; 

		entity_loader->LoadSkybox(skybox_params);

		if (load_default_scene)
		{
			model_parameters_t model_params{};
			model_params.model_path = "Resources/Models/Sponza/glTF/Sponza.gltf";
			model_params.textures_path = "Resources/Models/Sponza/glTF/";
			model_params.model_scale = 0.5f;

			//sun temple
			//model_params.model_path = "Resources/Models/SunTemple/suntemple.gltf";
			//model_params.textures_path = "Resources/Models/SunTemple/";
			//model_params.model_matrix = DirectX::XMMatrixRotationX(1.57079632679f);
			entity_loader->LoadGLTFModel(model_params);
		}

		light_parameters_t light_params{};
		light_params.light_data.casts_shadows = true;
		light_params.light_data.color = DirectX::XMVectorSet(1.0f, 0.9f, 0.99f, 1.0f);
		light_params.light_data.energy = 8;
		light_params.light_data.direction = DirectX::XMVectorSet(0.1f, -1.0f, 0.25f, 0.0f);
		light_params.light_data.type = ELightType::Directional;
		light_params.light_data.active = true;
		light_params.light_data.use_cascades = true;
		light_params.light_data.volumetric = false;
		light_params.light_data.volumetric_strength = 1.0f;
		light_params.mesh_type = ELightMesh::Quad;
		light_params.mesh_size = 250;
		
		entity_loader->LoadLight(light_params);

		Emitter test_emitter{};
		test_emitter.position = DirectX::XMFLOAT4(0, 150, 0, 1);
		test_emitter.velocity = DirectX::XMFLOAT4(0, 30, 0, 0);
		test_emitter.position_variance = DirectX::XMFLOAT4(5, 0, 5, 1);
		test_emitter.velocity_variance = 0.4f;
		test_emitter.number_to_emit = 0;
		test_emitter.particle_life_span = 40.0f;
		test_emitter.start_size = 1.0f;
		test_emitter.end_size = 0.3f;
		test_emitter.mass = 1.0f;
		test_emitter.particles_per_second = 40;
		test_emitter.particle_texture = renderer->GetTextureManager().LoadTexture("Resources/Textures/smoke.png");

		tecs::entity emitter = reg.create();
		reg.add(emitter, test_emitter);
	}
}