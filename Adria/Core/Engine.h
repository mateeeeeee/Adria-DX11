#pragma once
#include <memory>
#include <optional>
#include "../Events/EventQueue.h"
#include "../Input/Input.h"
#include "../tecs/registry.h"
#include "../Rendering/CameraManager.h"
#include "../Rendering/RendererSettings.h"
#include "../Rendering/SceneViewport.h"

namespace adria
{
	struct WindowMessage;
	class GraphicsDevice;
	class Renderer;
	class ModelImporter;
	class GUI;

	struct EngineInit
	{
		bool vsync = false;
		char const* scene_file = "scene.json";
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

		void HandleWindowMessage(WindowMessage const& msg_data);

		void Run(RendererSettings const& settings);
		void Present();

	private:
		bool vsync;
		tecs::registry reg;
		CameraManager camera_manager;
		Input input;
		std::unique_ptr<GraphicsDevice> gfx;
		std::unique_ptr<Renderer> renderer;
		std::unique_ptr<ModelImporter> entity_loader;

		SceneViewport scene_viewport_data;
		bool editor_active = true;

	private:

		virtual void InitializeScene(SceneConfig const& config);

		virtual void Update(float32 dt);

		virtual void Render(RendererSettings const& settings);

		void SetSceneViewportData(std::optional<SceneViewport> viewport_data);
	};
}