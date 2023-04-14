#pragma once
#include <memory>
#include "Core/Engine.h"
#include "GUI.h"
#include "tecs/registry.h"
#include "Rendering/RendererSettings.h"
#include "Rendering/SceneViewport.h"
#include "Graphics/GPUProfilerSettings.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/ImGuizmo.h"

namespace adria
{
    struct Material;
    struct ImGuiLogger;
    
	struct EditorInit
	{
		EngineInit engine_init;
	};
	class Editor
	{
		enum
		{
			Flag_Profiler,
			Flag_Camera,
			Flag_Log,
			Flag_Entities,
			Flag_HotReload,
			Flag_Renderer,
            Flag_Terrain,
			Flag_Ocean,
			Flag_Decal,
			Flag_Particles,
			Flag_Sky,
			Flag_AddEntities,
			Flag_Count
		};
	public:

        explicit Editor(EditorInit const& init);
        ~Editor();
        void HandleWindowMessage(WindowMessage const& msg_data);
        void Run();

	private:
        std::unique_ptr<Engine> engine;
		std::unique_ptr<GUI> gui;
        std::unique_ptr<ImGuiLogger> editor_log;
        tecs::entity selected_entity = tecs::null_entity;
        bool gizmo_enabled = false;
        bool scene_focused = false;
        ImGuizmo::OPERATION gizmo_op = ImGuizmo::TRANSLATE;
        RendererSettings renderer_settings{};
        GPUProfilerSettings profiler_settings{};
        SceneViewport scene_viewport_data;

        std::array<bool, Flag_Count> window_flags = { false };
	private:

        void SetStyle();
        void HandleInput();
        void MenuBar();
        void TerrainSettings();
        void OceanSettings();
        void SkySettings();
        void ParticleSettings();
        void DecalSettings();
        void ListEntities();
        void AddEntities();
        void Properties();
        void Camera();
        void Scene();
        void Log();
        void RendererSettings();
        void ShaderHotReload();
        void StatsAndProfiling();
	};
}


