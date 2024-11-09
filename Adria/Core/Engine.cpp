#include "Engine.h"
#include "Window.h"
#include "Math/Constants.h"
#include "Core/Logger.h"
#include "Core/Paths.h"
#include "Editor/ImGuiManager.h"
#include "Graphics/GfxDevice.h"
#include "Rendering/Renderer.h"
#include "Rendering/ModelImporter.h"
#include "Rendering/ShaderManager.h"
#include "Utilities/ThreadPool.h"
#include "Utilities/Random.h"
#include "Utilities/Timer.h"
#include "Utilities/JsonUtil.h"
#include "Utilities/StringUtil.h"
#include "Utilities/FilesUtil.h"

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
				JsonParams scene_params = json::parse(std::ifstream(paths::ScenesDir + scene_file));
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

				Float position[3] = { 0.0f, 0.0f, 0.0f };
				model_params.FindArray("translation", position);
				Matrix translation = XMMatrixTranslation(position[0], position[1], position[2]);

				Float angles[3] = { 0.0f, 0.0f, 0.0f };
				model_params.FindArray("rotation", angles);
				std::transform(std::begin(angles), std::end(angles), std::begin(angles), XMConvertToRadians);
				Matrix rotation = XMMatrixRotationX(angles[0]) * XMMatrixRotationY(angles[1]) * XMMatrixRotationZ(angles[2]);

				Float scale_factors[3] = { 1.0f, 1.0f, 1.0f };
				model_params.FindArray("scale", scale_factors);
				Matrix scale = XMMatrixScaling(scale_factors[0], scale_factors[1], scale_factors[2]);
				Matrix transform = rotation * scale * translation;

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
				Float position[3] = { 0.0f, 0.0f, 0.0f };
				light_params.FindArray("position", position);
				light.light_data.position = XMVectorSet(position[0], position[1], position[2], 1.0f);

				Float direction[3] = { 0.0f, -1.0f, 0.0f };
				light_params.FindArray("direction", direction);
				light.light_data.direction = XMVectorSet(direction[0], direction[1], direction[2], 0.0f);

				Float color[3] = { 1.0f, 1.0f, 1.0f };
				light_params.FindArray("color", color);
				light.light_data.color = XMVectorSet(color[0], color[1], color[2], 1.0f);

				light.light_data.energy = light_params.FindOr<Float>("energy", 1.0f);
				light.light_data.range = light_params.FindOr<Float>("range", 100.0f);

				light.light_data.outer_cosine = std::cos(XMConvertToRadians(light_params.FindOr<Float>("outer_angle", 45.0f)));
				light.light_data.inner_cosine = std::cos(XMConvertToRadians(light_params.FindOr<Float>("outer_angle", 22.5f)));

				light.light_data.casts_shadows = light_params.FindOr<Bool>("shadows", true);
				light.light_data.use_cascades = light_params.FindOr<Bool>("cascades", false);

				light.light_data.active = light_params.FindOr<Bool>("active", true);
				light.light_data.volumetric = light_params.FindOr<Bool>("volumetric", false);
				light.light_data.volumetric_strength = light_params.FindOr<Float>("volumetric_strength", 0.03f);

				light.light_data.lens_flare = light_params.FindOr<Bool>("lens_flare", false);
				light.light_data.god_rays = light_params.FindOr<Bool>("god_rays", false);

				light.light_data.godrays_decay = light_params.FindOr<Float>("godrays_decay", 0.825f);
				light.light_data.godrays_exposure = light_params.FindOr<Float>("godrays_exposure", 2.0f);
				light.light_data.godrays_density = light_params.FindOr<Float>("godrays_density", 0.975f);
				light.light_data.godrays_weight = light_params.FindOr<Float>("godrays_weight", 0.25f);

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
				light.mesh_size = light_params.FindOr<Uint32>("size", 100u);
				light.light_texture = light_params.FindOr<std::string>("texture", "");
				if (light.light_texture.has_value() && light.light_texture->empty()) light.light_texture = std::nullopt;

				if (type == "directional")
				{
					light.light_data.type = LightType::Directional;
				}
				else if (type == "point")
				{
					light.light_data.type = LightType::Point;
				}
				else if (type == "spot")
				{
					light.light_data.type = LightType::Spot;
				}
				else
				{
					ADRIA_LOG(WARNING, "Light has invalid type %s! Skipping this light...", type.c_str());
				}

				config.scene_lights.push_back(std::move(light));
			}

			JsonParams camera_params(camera);
			config.camera_params.near_plane = camera_params.FindOr<Float>("near", 1.0f);
			config.camera_params.far_plane  = camera_params.FindOr<Float>("far", 3000.0f);
			config.camera_params.fov = XMConvertToRadians(camera_params.FindOr<Float>("fov", 90.0f));
			config.camera_params.sensitivity = camera_params.FindOr<Float>("sensitivity", 0.3f);
			config.camera_params.speed = camera_params.FindOr<Float>("speed", 25.0f);

			Float position[3] = { 0.0f, 0.0f, 0.0f };
			camera_params.FindArray("position", position);
			config.camera_params.position = Vector3(position);

			Float look_at[3] = { 0.0f, 0.0f, 10.0f };
			camera_params.FindArray("look_at", look_at);
			config.camera_params.look_at = Vector3(look_at);

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
					config.skybox_params.cubemap = ToWideString(paths::TexturesDir) + L"Skybox/sunsetcube1024.dds";
				}
			}

			return config;
		}
	}


	using namespace tecs;

	Engine::Engine(EngineInit const& init) : window(init.window), vsync{ init.vsync }, scene_viewport_data{}
	{
		g_ThreadPool.Initialize();

		gfx = std::make_unique<GfxDevice>(window);
		g_TextureManager.Initialize(gfx.get());
		ShaderManager::Initialize(gfx.get());
		renderer = std::make_unique<Renderer>(reg, gfx.get(), window->Width(), window->Height());
		model_importer = std::make_unique<ModelImporter>(reg, gfx.get());

		InputEvents& input_events = g_Input.GetInputEvents();

		std::ignore = input_events.window_resized_event.AddMember(&GfxDevice::ResizeBackbuffer, *gfx);
		std::ignore = input_events.window_resized_event.AddMember(&Renderer::OnResize, *renderer);
		std::ignore = input_events.left_mouse_clicked.Add([this](Sint32 mx, Sint32 my) { renderer->OnLeftMouseClicked(); });
		std::ignore = input_events.f5_pressed_event.Add(ShaderManager::CheckIfShadersHaveChanged);

		std::optional<SceneConfig> scene_config = ParseSceneConfig(init.scene_file);
		if (scene_config.has_value())
		{
			InitializeScene(scene_config.value());
			scene_config.value().camera_params.aspect_ratio = static_cast<Float>(window->Width()) / window->Height();
			camera = std::make_unique<Camera>(scene_config.value().camera_params);
		}
		else window->Quit(1);

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
		g_ThreadPool.Destroy();
	}

	void Engine::OnWindowEvent(WindowEventData const& data)
	{
		g_Input.OnWindowEvent(data);
	}

	void Engine::Run(RendererSettings const& settings)
	{
		static AdriaTimer timer;
		Float const dt = timer.MarkInSeconds();

		g_Input.Tick();
		if (window->IsActive())
		{
			Update(dt);
			Render(settings);
		}
	}

	void Engine::Update(Float dt)
	{
		camera->Tick(dt);
		renderer->SetSceneViewportData(scene_viewport_data);
		renderer->Tick(camera.get());
		renderer->Update(dt);
	}

	void Engine::Render(RendererSettings const& settings)
	{
		renderer->Render(settings);
		if (editor_active)
		{
			renderer->ResolveToOffscreenTexture();
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
			
			scene_viewport_data.scene_viewport_pos_x = static_cast<Float>(window->PositionX());
			scene_viewport_data.scene_viewport_pos_y = static_cast<Float>(window->PositionY());
			scene_viewport_data.scene_viewport_size_x = static_cast<Float>(window->Width());
			scene_viewport_data.scene_viewport_size_y = static_cast<Float>(window->Height());
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