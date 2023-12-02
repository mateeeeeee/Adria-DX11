#pragma once
#include <memory>
#include <optional>
#include "Input.h"
#include "tecs/registry.h"
#include "Rendering/Camera.h"
#include "Rendering/RendererSettings.h"
#include "Rendering/SceneViewport.h"

namespace adria
{
	struct WindowEventData;
	class Window;
	class GfxDevice;
	class Renderer;
	class ModelImporter;
	class GUI;

	struct EngineInit
	{
		bool vsync = false;
		Window* window = nullptr;
		std::string scene_file = "scene.json";
	};

	struct SceneConfig;

	class Engine
	{
		friend class Editor;

	public:
		explicit Engine(EngineInit const&);
		Engine(Engine const&) = delete;
		Engine(Engine&&) = delete;
		Engine& operator=(Engine const&) = delete;
		Engine& operator=(Engine&&) = delete;
		~Engine();

		void OnWindowEvent(WindowEventData const&);

		void Run(RendererSettings const& settings);
		void Present();

	private:
		Window* window = nullptr;
		tecs::registry reg;
		std::unique_ptr<Camera> camera;
		std::unique_ptr<GfxDevice> gfx;
		std::unique_ptr<Renderer> renderer;
		std::unique_ptr<ModelImporter> model_importer;

		bool vsync;
		bool editor_active = true;
		SceneViewport scene_viewport_data;

	private:

		void InitializeScene(SceneConfig const& config);
		void Update(float dt);
		void Render(RendererSettings const& settings);
		void SetSceneViewportData(std::optional<SceneViewport> viewport_data);
	};
}