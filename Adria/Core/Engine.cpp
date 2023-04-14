#include "Engine.h"
#include "Window.h"
#include "../Math/Constants.h"
#include "../Logging/Logger.h"
#include "../Tasks/TaskManager.h"
#include "../Editor/GUI.h"
#include "../Graphics/GfxDevice.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/ModelImporter.h"
#include "../Rendering/ShaderManager.h"
#include "../Utilities/Random.h"
#include "../Utilities/Timer.h"
#include "../Utilities/JsonUtil.h"
#include "../Utilities/StringUtil.h"
#include "../Utilities/FilesUtil.h"

#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace DirectX;

namespace adria
{
	struct SceneConfig
	{
		std::vector<ModelParameters> scene_models;
		std::vector<LightParameters> scene_lights;
		SkyboxParameters skybox_params;
		CameraParameters camera_params;
	};

	namespace 
	{
		std::optional<SceneConfig> ParseSceneConfig(std::string const& scene_file)
		{
			SceneConfig config{};
			json models, lights, camera, skybox;
			try
			{
				JsonParams scene_params = json::parse(std::ifstream(scene_file));
				models = scene_params.FindJsonArray("models");
				lights = scene_params.FindJsonArray("lights");
				camera = scene_params.FindJson("camera");
				skybox = scene_params.FindJson("skybox");
			}
			catch (json::parse_error const& e)
			{
				ADRIA_LOG(ERROR, "JSON Parse error: %s! ", e.what());
				return std::nullopt;
			}

			for (auto&& model_json : models)
			{
				JsonParams model_params(model_json);

				std::string path;
				if (!model_params.Find<std::string>("path", path))
				{
					ADRIA_LOG(WARNING, "Model doesn't have path field! Skipping this model...");
				}
				std::string tex_path = model_params.FindOr<std::string>("tex_path", GetParentPath(path) + "\\");

				float32 position[3] = { 0.0f, 0.0f, 0.0f };
				model_params.FindArray("translation", position);
				XMMATRIX translation = XMMatrixTranslation(position[0], position[1], position[2]);

				float32 angles[3] = { 0.0f, 0.0f, 0.0f };
				model_params.FindArray("rotation", angles);
				std::transform(std::begin(angles), std::end(angles), std::begin(angles), XMConvertToRadians);
				XMMATRIX rotation = XMMatrixRotationX(angles[0]) * XMMatrixRotationY(angles[1]) * XMMatrixRotationZ(angles[2]);

				float32 scale_factors[3] = { 1.0f, 1.0f, 1.0f };
				model_params.FindArray("scale", scale_factors);
				XMMATRIX scale = XMMatrixScaling(scale_factors[0], scale_factors[1], scale_factors[2]);
				XMMATRIX transform = rotation * scale * translation;

				config.scene_models.emplace_back(path, tex_path, transform);
			}

			for (auto&& light_json : lights)
			{
				JsonParams light_params(light_json);

				std::string type;
				if (!light_params.Find<std::string>("type", type))
				{
					ADRIA_LOG(WARNING, "Light doesn't have type field! Skipping this light...");
				}

				LightParameters light{};
				float32 position[3] = { 0.0f, 0.0f, 0.0f };
				light_params.FindArray("position", position);
				light.light_data.position = XMVectorSet(position[0], position[1], position[2], 1.0f);

				float32 direction[3] = { 0.0f, -1.0f, 0.0f };
				light_params.FindArray("direction", direction);
				light.light_data.direction = XMVectorSet(direction[0], direction[1], direction[2], 0.0f);

				float32 color[3] = { 1.0f, 1.0f, 1.0f };
				light_params.FindArray("color", color);
				light.light_data.color = XMVectorSet(color[0], color[1], color[2], 1.0f);

				light.light_data.energy = light_params.FindOr<float32>("energy", 1.0f);
				light.light_data.range = light_params.FindOr<float32>("range", 100.0f);

				light.light_data.outer_cosine = std::cos(XMConvertToRadians(light_params.FindOr<float32>("outer_angle", 45.0f)));
				light.light_data.inner_cosine = std::cos(XMConvertToRadians(light_params.FindOr<float32>("outer_angle", 22.5f)));

				light.light_data.casts_shadows = light_params.FindOr<bool>("shadows", true);
				light.light_data.use_cascades = light_params.FindOr<bool>("cascades", false);

				light.light_data.active = light_params.FindOr<bool>("active", true);
				light.light_data.volumetric = light_params.FindOr<bool>("volumetric", false);
				light.light_data.volumetric_strength = light_params.FindOr<float32>("volumetric_strength", 1.0f);

				light.light_data.lens_flare = light_params.FindOr<bool>("lens_flare", false);
				light.light_data.god_rays = light_params.FindOr<bool>("god_rays", false);

				light.light_data.godrays_decay = light_params.FindOr<float32>("godrays_decay", 0.825f);
				light.light_data.godrays_exposure = light_params.FindOr<float32>("godrays_exposure", 2.0f);
				light.light_data.godrays_density = light_params.FindOr<float32>("godrays_density", 0.975f);
				light.light_data.godrays_weight = light_params.FindOr<float32>("godrays_weight", 0.25f);

				light.mesh_type = LightMesh::NoMesh;
				std::string mesh = light_params.FindOr<std::string>("mesh", "");
				if (mesh == "cube")
				{
					light.mesh_type = LightMesh::Cube;
				}
				else if (mesh == "quad")
				{
					light.mesh_type = LightMesh::Quad;
				}
				light.mesh_size = light_params.FindOr<uint32>("size", 100u);
				light.light_texture = light_params.FindOr<std::string>("texture", "");
				if (light.light_texture.has_value() && light.light_texture->empty()) light.light_texture = std::nullopt;

				if (type == "directional")
				{
					light.light_data.type = ELightType::Directional;
				}
				else if (type == "point")
				{
					light.light_data.type = ELightType::Point;
				}
				else if (type == "spot")
				{
					light.light_data.type = ELightType::Spot;
				}
				else
				{
					ADRIA_LOG(WARNING, "Light has invalid type %s! Skipping this light...", type.c_str());
				}

				config.scene_lights.push_back(std::move(light));
			}

			JsonParams camera_params(camera);
			config.camera_params.near_plane = camera_params.FindOr<float32>("near", 1.0f);
			config.camera_params.far_plane  = camera_params.FindOr<float32>("far", 3000.0f);
			config.camera_params.fov = XMConvertToRadians(camera_params.FindOr<float32>("fov", 90.0f));
			config.camera_params.sensitivity = camera_params.FindOr<float32>("sensitivity", 0.3f);
			config.camera_params.speed = camera_params.FindOr<float32>("speed", 25.0f);

			float32 position[3] = { 0.0f, 0.0f, 0.0f };
			camera_params.FindArray("position", position);
			config.camera_params.position = XMFLOAT3(position);

			float32 look_at[3] = { 0.0f, 0.0f, 10.0f };
			camera_params.FindArray("look_at", look_at);
			config.camera_params.look_at = XMFLOAT3(look_at);

			JsonParams skybox_params(skybox);
			std::vector<std::string> skybox_textures;
			
			std::string cubemap[1];
			if (skybox_params.FindArray("texture", cubemap))
			{
				config.skybox_params.cubemap = ToWideString(cubemap[0]);
			}
			else
			{
				std::string cubemap[6];
				if (skybox_params.FindArray("texture", cubemap))
				{
					config.skybox_params.cubemap_textures = std::to_array(cubemap);
				}
				else
				{
					ADRIA_LOG(WARNING, "Skybox texture not found or is incorrectly specified!  \
										Size of texture array has to be either 1 or 6! Fallback to the default one...");
					config.skybox_params.cubemap = L"Resources/Textures/Skybox/sunsetcube1024.dds";
				}
			}

			return config;
		}
	}


	using namespace tecs;

	Engine::Engine(EngineInit const& init) : vsync{ init.vsync }, scene_viewport_data{}
	{
		g_TaskManager.Initialize();

		gfx = std::make_unique<GfxDevice>(Window::Handle());
		g_TextureManager.Initialize(gfx.get());
		ShaderManager::Initialize(gfx->Device());
		renderer = std::make_unique<Renderer>(reg, gfx.get(), Window::Width(), Window::Height());
		model_importer = std::make_unique<ModelImporter>(reg, gfx.get());

		InputEvents& input_events = g_Input.GetInputEvents();

		std::ignore = input_events.window_resized_event.AddMember(&GfxDevice::ResizeBackbuffer, *gfx);
		std::ignore = input_events.window_resized_event.AddMember(&Renderer::OnResize, *renderer);
		std::ignore = input_events.left_mouse_clicked_event.Add([this](int32 mx, int32 my) { renderer->OnLeftMouseClicked(); });
		std::ignore = input_events.f5_pressed_event.Add(ShaderManager::CheckIfShadersHaveChanged);

		std::optional<SceneConfig> scene_config = ParseSceneConfig(init.scene_file);
		if (scene_config.has_value())
		{
			InitializeScene(scene_config.value());
			scene_config.value().camera_params.aspect_ratio = static_cast<float32>(Window::Width()) / Window::Height();
			camera = std::make_unique<Camera>(scene_config.value().camera_params);
		}
		else Window::Quit(1);

		std::ignore = input_events.window_resized_event.AddMember(&Camera::OnResize, *camera);
		std::ignore = input_events.scroll_mouse_event.AddMember(&Camera::Zoom, *camera);
	}

	Engine::~Engine()
	{
		model_importer = nullptr;
		renderer = nullptr;
		ShaderManager::Destroy();
		g_TextureManager.Destroy();
		gfx = nullptr;
		g_TaskManager.Destroy();
	}

	void Engine::HandleWindowMessage(WindowMessage const& msg_data)
	{
		g_Input.HandleWindowMessage(msg_data);
	}

	void Engine::Run(RendererSettings const& settings)
	{
		static AdriaTimer timer;
		float32 const dt = timer.MarkInSeconds();

		g_Input.NewFrame();
		if (Window::IsActive())
		{
			Update(dt);
			Render(settings);
		}
	}

	void Engine::Update(float32 dt)
	{
		camera->Tick(dt);
		renderer->SetSceneViewportData(std::move(scene_viewport_data));
		renderer->NewFrame(camera.get());
		renderer->Update(dt);
	}

	void Engine::Render(RendererSettings const& settings)
	{
		renderer->Render(settings);
		if (editor_active)
		{
			renderer->ResolveToOffscreenFramebuffer();
		}
		else
		{
			gfx->ClearBackbuffer();
			renderer->ResolveToBackbuffer();
		}
	}

	void Engine::SetSceneViewportData(std::optional<SceneViewport> viewport_data)
	{
		if (viewport_data.has_value())
		{
			editor_active = true;
			scene_viewport_data = viewport_data.value();
		}
		else
		{
			editor_active = false;
			scene_viewport_data.scene_viewport_focused = true;
			scene_viewport_data.mouse_position_x = g_Input.GetMousePositionX();
			scene_viewport_data.mouse_position_y = g_Input.GetMousePositionY();
			
			auto [pos_x, pos_y] = Window::Position();
			scene_viewport_data.scene_viewport_pos_x = static_cast<float32>(pos_x);
			scene_viewport_data.scene_viewport_pos_y = static_cast<float32>(pos_y);
			scene_viewport_data.scene_viewport_size_x = static_cast<float32>(Window::Width());
			scene_viewport_data.scene_viewport_size_y = static_cast<float32>(Window::Height());
		}
	}

	void Engine::Present()
	{
		gfx->SwapBuffers(vsync);
	}

	void Engine::InitializeScene(SceneConfig const& config)
	{
		model_importer->LoadSkybox(config.skybox_params);
		for (auto&& model : config.scene_models) model_importer->ImportModel_GLTF(model);
		for (auto&& light : config.scene_lights) model_importer->LoadLight(light);
	}
}