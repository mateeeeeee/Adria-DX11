#include "Editor.h"
#include "../Rendering/Renderer.h"
#include "../Core/Events.h"
#include "../Graphics/GraphicsCoreDX11.h"
#include "../Rendering/EntityLoader.h"
#include "../Logging/Logger.h"
#include "../Utilities/FilesUtil.h"
#include "../Utilities/StringUtil.h"
#include "../Utilities/Image.h"
#include "../ImGui/ImGuiFileDialog.h"
#include "../Utilities/Random.h"
#include "../Math/BoundingVolumeHelpers.h"


using namespace DirectX;

namespace adria
{

	using namespace tecs;

	enum class EMaterialTextureType
	{
		Albedo,
		MetallicRoughness,
		Emissive
	};

	struct ImGuiLogger
	{
		ImGuiTextBuffer     Buf;
		ImGuiTextFilter     Filter;
		ImVector<int>       LineOffsets;
		bool                AutoScroll;

		ImGuiLogger()
		{
			AutoScroll = true;
			Clear();
		}

		void Clear()
		{
			Buf.clear();
			LineOffsets.clear();
			LineOffsets.push_back(0);
		}

		void AddLog(const char* fmt, ...) IM_FMTARGS(2)
		{
			int old_size = Buf.size();
			va_list args;
			va_start(args, fmt);
			Buf.appendfv(fmt, args);
			va_end(args);
			for (int new_size = Buf.size(); old_size < new_size; old_size++)
				if (Buf[old_size] == '\n')
					LineOffsets.push_back(old_size + 1);
		}

		void Draw(const char* title, bool* p_open = NULL)
		{
			if (!ImGui::Begin(title, p_open))
			{
				ImGui::End();
				return;
			}

			if (ImGui::BeginPopup("Options"))
			{
				ImGui::Checkbox("Auto-scroll", &AutoScroll);
				ImGui::EndPopup();
			}

			if (ImGui::Button("Options"))
				ImGui::OpenPopup("Options");
			ImGui::SameLine();
			bool clear = ImGui::Button("Clear");
			ImGui::SameLine();
			bool copy = ImGui::Button("Copy");
			ImGui::SameLine();
			Filter.Draw("Filter", -100.0f);

			ImGui::Separator();
			ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

			if (clear)
				Clear();
			if (copy)
				ImGui::LogToClipboard();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			const char* buf = Buf.begin();
			const char* buf_end = Buf.end();
			if (Filter.IsActive())
			{
				for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
				{
					const char* line_start = buf + LineOffsets[line_no];
					const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
					if (Filter.PassFilter(line_start, line_end))
						ImGui::TextUnformatted(line_start, line_end);
				}
			}
			else
			{
				ImGuiListClipper clipper;
				clipper.Begin(LineOffsets.Size);
				while (clipper.Step())
				{
					for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
					{
						const char* line_start = buf + LineOffsets[line_no];
						const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
						ImGui::TextUnformatted(line_start, line_end);
					}
				}
				clipper.End();
			}
			ImGui::PopStyleVar();

			if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);

			ImGui::EndChild();
			ImGui::End();
		}
	};

    class EditorLogger : public ILogger
    {
    public:
        EditorLogger(ImGuiLogger* logger, ELogLevel logger_level = ELogLevel::LOG_DEBUG) : logger{ logger }, logger_level{ logger_level } {}

        virtual void Log(ELogLevel level, char const* entry, char const* file, uint32_t line) override
        {
			if (level < logger_level) return;
			std::string log_entry = GetLogTime() + LevelToString(level) + std::string(entry) + "\n";
            if (logger) logger->AddLog(log_entry.c_str());
        }
    private:
        ImGuiLogger* logger;
        ELogLevel logger_level;
    };


    Editor::Editor(editor_init_t const& init) : engine(), editor_log(new ImGuiLogger{})
    {
        engine = std::make_unique<Engine>(init.engine_init);
        gui = std::make_unique<GUI>(engine->gfx.get());
        ADRIA_REGISTER_LOGGER(new EditorLogger(editor_log.get()));

        SetStyle();
    }

    Editor::~Editor()
    {
    }

	void Editor::HandleWindowMessage(window_message_t const& msg_data)
    {
        engine->HandleWindowMessage(msg_data);
        gui->HandleWindowMessage(msg_data);
    }

    void Editor::Run()
    {
        HandleInput();

        if (gui->IsVisible())
        {
            engine->Run(renderer_settings, true);
            engine->gfx->ClearBackbuffer();
            engine->gfx->SetBackbuffer();
            gui->Begin();
            {
				ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
                MenuBar();
                ListEntities();
                TerrainSettings();
                OceanSettings();
                SkySettings();
                ParticleSettings();
                Camera();
                Scene();
                RendererSettings();
                Properties();
                Log();
                StatsAndProfiling();
            }
            gui->End();
            engine->Present();
        }
        else
        {
            engine->Run(renderer_settings, false);
            engine->Present();
        }
    }

    void Editor::SetStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.GrabRounding = 0.f;
        style.WindowRounding = 0.f;
        style.ScrollbarRounding = 3.f;
        style.FrameRounding = 3.f;
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

        style.Colors[ImGuiCol_Text] = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.26f, 0.26f, 0.26f, 0.95f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.43f, 0.43f, 0.43f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.32f, 0.52f, 0.65f, 1.00f);
       
    }

    void Editor::HandleInput()
    {
        if (mouse_in_scene && engine->input.IsKeyDown(KeyCode::I)) gui->ToggleVisibility();

        if (mouse_in_scene && engine->input.IsKeyDown(KeyCode::G)) gizmo_enabled = !gizmo_enabled;

        if (gizmo_enabled && gui->IsVisible())
        {

            if (engine->input.IsKeyDown(KeyCode::T)) gizmo_op = ImGuizmo::TRANSLATE;

            if (engine->input.IsKeyDown(KeyCode::R)) gizmo_op = ImGuizmo::ROTATE;

            if (engine->input.IsKeyDown(KeyCode::E)) gizmo_op = ImGuizmo::SCALE; //e because s is for camera movement and its close to wasd and tr

        }

        engine->camera_manager.ShouldUpdate(mouse_in_scene);
    }

    void Editor::MenuBar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Model"))
                    ImGuiFileDialog::Instance()->OpenDialog("Choose Model", "Choose File", ".gltf", ".");

                ImGui::EndMenu();
            }

            if (ImGuiFileDialog::Instance()->Display("Choose Model"))
            {

                if (ImGuiFileDialog::Instance()->IsOk())
                {
                    std::string model_path = ImGuiFileDialog::Instance()->GetFilePathName();

                    model_parameters_t params{};
                    params.model_path = model_path;
                    std::string texture_path = ImGuiFileDialog::Instance()->GetCurrentPath();
                    if (!texture_path.empty()) texture_path.append("/");

                    params.textures_path = texture_path;
                    engine->entity_loader->LoadGLTFModel(params);
                }

                ImGuiFileDialog::Instance()->Close();
            }

            if (ImGui::BeginMenu("Help"))
            {
                ImGui::Text("Controls\n");
                ImGui::Text(
                    "Move Camera with W, A, S, D, Q and E. Use Mouse for Rotating Camera. Use Mouse Scroll for Zoom In/Out.\n"
                    "Press I to toggle between Cinema Mode and Editor Mode. (Scene Window has to be active) \n"
                    "Press G to toggle Gizmo. (Scene Window has to be active) \n"
                    "When Gizmo is enabled, use T, R and E to switch between Translation, Rotation and Scaling Mode.\n"
                    "Left Click on entity to select it. Left click again on selected entity to unselect it.\n"
                    "Right Click on empty area in Entities window to add entity. Right Click on selected entity to delete it.\n" 
                   
                );
                ImGui::Spacing();

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

	void Editor::TerrainSettings()
	{
		ImGui::Begin("Terrain");
		{
            if (ImGui::TreeNodeEx("Terrain Generation", 0))
            {
				static grid_parameters_t terrain_params{};
				static i32 tile_count[2] = { 600, 600 };
				static f32 tile_size[2] = { 2.0f, 2.0f };
				static f32 texture_scale[2] = { 200.0f, 200.0f };
				static i32 chunk_count[2] = { 40, 40 };
				static bool split_to_chunks = false;

				ImGui::SliderInt2("Tile Count", tile_count, 32, 2048);
				ImGui::SliderFloat2("Tile Size", tile_size, 0.1f, 20.0f);
				ImGui::SliderFloat2("Texture Scale", texture_scale, 1.0f, 400.0f);

				ImGui::Checkbox("Split Mesh to Chunks", &split_to_chunks);
				if (split_to_chunks)
				{
					ImGui::SliderInt2("Chunk Count", chunk_count, 8, 64);
				}

				static f32 max_height = 300;
				static bool procedural_generation = true;
				ImGui::SliderFloat("Max Height", &max_height, 10.0f, 2000.0f);
				ImGui::Checkbox("Procedural Generation", &procedural_generation);

				static noise_desc_t noise_desc
				{
				 .fractal_type = EFractalType::FBM,
				 .noise_type = ENoiseType::Perlin,
				 .seed = 33,
				 .persistence = 0.666f,
				 .lacunarity = 2.0f,
				 .octaves = 4,
				 .noise_scale = 24
				};
				if (procedural_generation)
				{
					noise_desc.width = (u32)tile_count[0] + 1;
					noise_desc.depth = (u32)tile_count[1] + 1;
					noise_desc.max_height = (u32)max_height;

					ImGui::SliderInt("Seed", &noise_desc.seed, 1, 1000000);
					ImGui::SliderInt("Octaves", &noise_desc.octaves, 1, 16);
					ImGui::SliderFloat("Persistence", &noise_desc.persistence, 0.0f, 1.0f);
					ImGui::SliderFloat("Lacunarity", &noise_desc.lacunarity, 0.1f, 8.0f);
					ImGui::SliderFloat("Frequency", &noise_desc.frequency, 0.01f, 0.2f);
					ImGui::SliderFloat("Noise Scale", &noise_desc.noise_scale, 1, 128);

					const char* noise_types[] = { "OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "ValueCubic", "Value" };
					static int current_noise_type = 3;
					const char* noise_combo_label = noise_types[current_noise_type];
					if (ImGui::BeginCombo("Noise Type", noise_combo_label, 0))
					{
						for (int n = 0; n < IM_ARRAYSIZE(noise_types); n++)
						{
							const bool is_selected = (current_noise_type == n);
							if (ImGui::Selectable(noise_types[n], is_selected)) current_noise_type = n;
							if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					noise_desc.noise_type = static_cast<ENoiseType>(current_noise_type);

					const char* fractal_types[] = { "None", "FBM", "Ridged", "PingPong" };
					static int current_fractal_type = 1;
					const char* fractal_combo_label = fractal_types[current_fractal_type];
					if (ImGui::BeginCombo("Fractal Type", fractal_combo_label, 0))
					{
						for (int n = 0; n < IM_ARRAYSIZE(fractal_types); n++)
						{
							const bool is_selected = (current_fractal_type == n);
							if (ImGui::Selectable(fractal_types[n], is_selected)) current_fractal_type = n;
							if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					noise_desc.fractal_type = static_cast<EFractalType>(current_fractal_type);
				}
				else
				{
					//todo
					//add heightmap and splat map texture choice
				}

                static bool thermal_erosion = false;
                static thermal_erosion_desc_t thermal_erosion_desc{.iterations = 3, .c = 0.5f, .talus = 0.025f};
				ImGui::Checkbox("Thermal Erosion", &thermal_erosion);
                if (thermal_erosion && ImGui::TreeNodeEx("Thermal Erosion Settings", 0))
                {
					ImGui::SliderInt("Iterations", &thermal_erosion_desc.iterations, 1, 10);
                    ImGui::SliderFloat("C", &thermal_erosion_desc.c, 0.01f, 1.0f);
                    ImGui::SliderFloat("Talus", &thermal_erosion_desc.talus, 4.0f / std::max(tile_count[0], tile_count[1]), 16.0f / std::max(tile_count[0], tile_count[1]));

					ImGui::TreePop();
					ImGui::Separator();
                }

				static bool hydraulic_erosion = false;
				static hydraulic_erosion_desc_t hydraulic_erosion_desc{ .iterations = 3, .drops = 1000000, .carrying_capacity = 1.5f, .deposition_speed = 0.03f };
				ImGui::Checkbox("Hydraulic Erosion", &hydraulic_erosion);
				if (hydraulic_erosion && ImGui::TreeNodeEx("Hydraulic Erosion Settings", 0))
				{
					ImGui::SliderInt("Iterations", &hydraulic_erosion_desc.iterations, 1, 10);
					ImGui::SliderInt("Raindrops", &hydraulic_erosion_desc.drops, 10000, 5000000);
					ImGui::SliderFloat("Carrying capacity", &hydraulic_erosion_desc.carrying_capacity, 0.5f, 2.0f);
					ImGui::SliderFloat("Deposition Speed", &hydraulic_erosion_desc.deposition_speed, 0.01f, 0.1f);

					ImGui::TreePop();
					ImGui::Separator();
				}

                static terrain_texture_layer_parameters_t layer_params{};
				if (ImGui::TreeNodeEx("Layer Texture Settings", 0))
				{
					ImGui::SliderFloat("Sand Start", &layer_params.terrain_sand_start, -1000.0f, 10.0f);
					ImGui::SliderFloat("Sand End", &layer_params.terrain_sand_end, layer_params.terrain_sand_start, 25.0f);
                    ImGui::SliderFloat("Grass Start", &layer_params.terrain_grass_start, 0.0f, 100.0f);
                    ImGui::SliderFloat("Grass End", &layer_params.terrain_grass_end, layer_params.terrain_grass_start, 1000.0f);

                    ImGui::SliderFloat("Rock Start", &layer_params.terrain_rocks_start, 0.0f, 100.0f);
                    ImGui::SliderFloat("Slope Grass Start", &layer_params.terrain_slope_grass_start, 0.0f, 1.0f);
                    ImGui::SliderFloat("Slope Rocks Start", &layer_params.terrain_slope_rocks_start, 0.0f, 1.0f);

                    ImGui::SliderFloat("Terrain Height Mix Zone", &layer_params.height_mix_zone, 0.0f, 100.0f);
                    ImGui::SliderFloat("Terrain Slope Mix Zone", &layer_params.slope_mix_zone, 0.0f, 0.1f);

					ImGui::TreePop();
					ImGui::Separator();
				}

				if (ImGui::Button("Generate Terrain"))
				{
					engine->reg.destroy<TerrainComponent>();

					terrain_parameters_t params{};
					terrain_params.tile_size_x = tile_size[0];
					terrain_params.tile_size_z = tile_size[1];
					terrain_params.tile_count_x = tile_count[0];
					terrain_params.tile_count_z = tile_count[1];
					terrain_params.texture_scale_x = texture_scale[0];
					terrain_params.texture_scale_z = texture_scale[1];
					terrain_params.split_to_chunks = split_to_chunks;
					terrain_params.chunk_count_x = chunk_count[0];
					terrain_params.chunk_count_z = chunk_count[1];
					terrain_params.normal_type = ENormalCalculation::AreaWeight;
					terrain_params.heightmap = std::make_unique<Heightmap>(noise_desc);
                    if (thermal_erosion) terrain_params.heightmap->ApplyThermalErosion(thermal_erosion_desc);
                    if (hydraulic_erosion) terrain_params.heightmap->ApplyHydraulicErosion(hydraulic_erosion_desc);

					params.terrain_grid = std::move(terrain_params);
                    params.layer_params = layer_params;
					params.grass_texture = "Resources/Textures/Terrain/terrain_grass.dds";
					params.rock_texture = "Resources/Textures/Terrain/grass2.dds";
					params.base_texture = "Resources/Textures/Terrain/terrain_grass.dds";
					params.sand_texture = "Resources/Textures/Terrain/mud.jpg";
					static const char* layer_texture_name = "layer.tga";
					params.layer_texture = layer_texture_name;

					engine->entity_loader->LoadTerrain(params);
				}

				ImGui::TreePop();
				ImGui::Separator();
            }

            if (ImGui::TreeNodeEx("Foliage Generation", 0))
            {
				static std::vector<foliage_parameters_t> foliages{};
				static foliage_parameters_t foliage_params{
					.foliage_count = 3000,
					.foliage_scale = 10,
                    .foliage_height_start = 0.0f,
					.foliage_height_end = 1000.0f,
					.foliage_slope_start = 0.95f };
				if (ImGui::TreeNode("Foliage Settings"))
				{
					ImGui::SliderInt("Foliage Count", &foliage_params.foliage_count, 100, 25000);
					ImGui::SliderFloat("Foliage Scale", &foliage_params.foliage_scale, 1.0f, 100.0f);
					ImGui::SliderFloat("Foliage Slope Start", &foliage_params.foliage_slope_start, 0.0f, 1.0f);
					ImGui::SliderFloat("Foliage Height Start", &foliage_params.foliage_height_start, -50.0f, 50.0f);
					ImGui::SliderFloat("Foliage Height End", &foliage_params.foliage_height_end, -100.0f, 1000.0f);

					const char* foliage_types[] = { "Single Quad", "Double Quad", "Triple Quad" };
					static int current_foliage_type = 0;
					const char* foliage_combo_label = foliage_types[current_foliage_type];
					if (ImGui::BeginCombo("Foliage Type", foliage_combo_label, 0))
					{
						for (int n = 0; n < IM_ARRAYSIZE(foliage_types); n++)
						{
							const bool is_selected = (current_foliage_type == n);
							if (ImGui::Selectable(foliage_types[n], is_selected)) current_foliage_type = n;
							if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					foliage_params.mesh_texture_pair.first = static_cast<EFoliageMesh>(current_foliage_type);

					if (ImGui::Button("Add Foliage"))
					{
						static std::string foliage_textures[] = {
							"Resources/Textures/Foliage/foliage.png",
                            "Resources/Textures/Foliage/foliage2.png",
							"Resources/Textures/Foliage/foliage3.png",
							"Resources/Textures/Foliage/foliage4.png",
							"Resources/Textures/Foliage/grass_flower_type1.png",
							"Resources/Textures/Foliage/grass_flower_type3.png",
							"Resources/Textures/Foliage/grass_flower_type10.png",
							"Resources/Textures/Foliage/grass_type1.png",
							"Resources/Textures/Foliage/grass_type2.png",
							"Resources/Textures/Foliage/grass_type3.png",
							"Resources/Textures/Foliage/grass_type4.png",
							"Resources/Textures/Foliage/grass_type6.png",
						};
						static IntRandomGenerator<size_t> index(0ll, ARRAYSIZE(foliage_textures) - 1);

						foliage_params.mesh_texture_pair.second = foliage_textures[index()];
						foliages.push_back(foliage_params);
					}

					ImGui::TreePop();
					ImGui::Separator();
				}
				ImGui::Text("Number of foliages to add: %d", foliages.size());
				if (ImGui::Button("Generate Foliage"))
				{
					if (TerrainComponent::terrain != nullptr)
					{
						auto [tile_size_x, tile_size_z] = TerrainComponent::terrain->TileSizes();
						auto [tile_count_x, tile_count_z] = TerrainComponent::terrain->TileCounts();
						for (auto& foliage_params : foliages)
						{
							foliage_params.foliage_center.x = tile_size_x * tile_count_x / 2;
							foliage_params.foliage_center.y = tile_size_z * tile_count_z / 2;
							foliage_params.foliage_extents.x = tile_size_x * tile_count_x / 2;
							foliage_params.foliage_extents.y = tile_size_z * tile_count_z / 2;

							engine->entity_loader->LoadFoliage(foliage_params);
						}

						foliages.clear();
					}
				}

				ImGui::TreePop();
				ImGui::Separator();
            }

			if (ImGui::TreeNodeEx("Tree Generation", 0))
			{
				static std::vector<tree_parameters_t> trees{};
				static tree_parameters_t tree_params{
					.tree_count = 5,
					.tree_scale = 50,
					.tree_height_start = 0.0f,
					.tree_height_end = 1000.0f,
					.tree_slope_start = 0.95f };
				if (ImGui::TreeNode("Tree Settings"))
				{
					ImGui::SliderInt("Tree Count", &tree_params.tree_count, 1, 50);
					ImGui::SliderFloat("Tree Scale", &tree_params.tree_scale, 10.0f, 100.0f);
					ImGui::SliderFloat("Tree Slope Start", &tree_params.tree_slope_start, 0.0f, 1.0f);
					ImGui::SliderFloat("Tree Height Start", &tree_params.tree_height_start, 10.0f, 200.0f);
					ImGui::SliderFloat("Tree Height End", &tree_params.tree_height_end, 200.0f, 1000.0f);

					const char* tree_types[] = { "Tree01", "Tree02"};
					static int current_tree_type = 0;
					const char* tree_combo_label = tree_types[current_tree_type];
					if (ImGui::BeginCombo("Tree Type", tree_combo_label, 0))
					{
						for (int n = 0; n < IM_ARRAYSIZE(tree_types); n++)
						{
							const bool is_selected = (current_tree_type == n);
							if (ImGui::Selectable(tree_types[n], is_selected)) current_tree_type = n;
							if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
					tree_params.tree_type = static_cast<ETreeType>(current_tree_type);

					if (ImGui::Button("Add Trees"))
					{
                        trees.push_back(tree_params);
					}

					ImGui::TreePop();
					ImGui::Separator();
				}
				ImGui::Text("Number of trees to add: %d", trees.size());
				if (ImGui::Button("Generate Trees"))
				{
					if (TerrainComponent::terrain != nullptr)
					{
						auto [tile_size_x, tile_size_z] = TerrainComponent::terrain->TileSizes();
						auto [tile_count_x, tile_count_z] = TerrainComponent::terrain->TileCounts();
						for (auto& tree_params : trees)
						{
							tree_params.tree_center.x = tile_size_x * tile_count_x / 2;
							tree_params.tree_center.y = tile_size_z * tile_count_z / 2;
							tree_params.tree_extents.x = tile_size_x * tile_count_x / 2;
							tree_params.tree_extents.y = tile_size_z * tile_count_z / 2;

							engine->entity_loader->LoadTrees(tree_params);
						}

                        trees.clear();
					}
				}

				ImGui::TreePop();
				ImGui::Separator();
			}


			if (ImGui::Button("Clear"))
			{
				engine->reg.destroy<TerrainComponent>();
                TerrainComponent::terrain = nullptr;
			}
			if (ImGui::TreeNodeEx("Terrain Settings", 0))
			{
				ImGui::TreePop();
				ImGui::Separator();
			}

		}
		ImGui::End();
	}

	void Editor::OceanSettings()
    {
        ImGui::Begin("Ocean");
        {
			static grid_parameters_t ocean_params{};
			static i32 tile_count[2] = { 512, 512 };
			static f32 tile_size[2] = { 40.0f, 40.0f };
			static f32 texture_scale[2] = { 20.0f, 20.0f };

			ImGui::SliderInt2("Tile Count", tile_count, 32, 1024);
			ImGui::SliderFloat2("Tile Size", tile_size, 1.0, 100.0f);
			ImGui::SliderFloat2("Texture Scale", texture_scale, 0.1f, 10.0f);

			ocean_params.tile_count_x = tile_count[0];
			ocean_params.tile_count_z = tile_count[1];
			ocean_params.tile_size_x = tile_size[0];
			ocean_params.tile_size_z = tile_size[1];
			ocean_params.texture_scale_x = texture_scale[0];
			ocean_params.texture_scale_z = texture_scale[1];

			if (ImGui::Button("Load Ocean"))
			{
				ocean_parameters_t params{};
				params.ocean_grid = std::move(ocean_params);
				engine->entity_loader->LoadOcean(params);
			}

			if (ImGui::Button("Clear"))
			{
				engine->reg.destroy<Ocean>();
			}

			if (ImGui::TreeNodeEx("Ocean Settings", 0))
			{
				ImGui::Checkbox("Tessellation", &renderer_settings.ocean_tesselation);
				ImGui::Checkbox("Wireframe", &renderer_settings.ocean_wireframe);

				ImGui::SliderFloat("Choppiness", &renderer_settings.ocean_choppiness, 0.0f, 10.0f);
				renderer_settings.ocean_color_changed = ImGui::ColorEdit3("Ocean Color", renderer_settings.ocean_color);
				ImGui::TreePop();
				ImGui::Separator();
			}
        }
        ImGui::End();
    }

	void Editor::SkySettings()
	{
        ImGui::Begin("Sky");
        {
			const char* sky_types[] = { "Skybox", "Uniform Color", "Hosek-Wilkie" };
			static int current_sky_type = 0;
			const char* combo_label = sky_types[current_sky_type];
			if (ImGui::BeginCombo("Sky Type", combo_label, 0))
			{
				for (int n = 0; n < IM_ARRAYSIZE(sky_types); n++)
				{
					const bool is_selected = (current_sky_type == n);
					if (ImGui::Selectable(sky_types[n], is_selected)) current_sky_type = n;
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (current_sky_type == 0) renderer_settings.sky_type = ESkyType::Skybox;
			else if (current_sky_type == 1)
			{
				renderer_settings.sky_type = ESkyType::UniformColor;
				static char const* const sky_colors[] = { "Deep Sky Blue", "Sky Blue", "Light Sky Blue" };
				static int current_sky_color = 0;
				ImGui::ListBox("Tone Map Operator", &current_sky_color, sky_colors, IM_ARRAYSIZE(sky_colors));

				switch (current_sky_color)
				{
				case 0:
				{
					static f32 deep_sky_blue[3] = { 0.0f, 0.75f, 1.0f };
					memcpy(renderer_settings.sky_color, deep_sky_blue, sizeof(deep_sky_blue));
					break;
				}
				case 1:
				{
					static f32 sky_blue[3] = { 0.53f, 0.81f, 0.92f };
					memcpy(renderer_settings.sky_color, sky_blue, sizeof(sky_blue));
					break;
				}
				case 2:
				{
					static f32 light_sky_blue[3] = { 0.53f, 0.81f, 0.98f };
					memcpy(renderer_settings.sky_color, light_sky_blue, sizeof(light_sky_blue));
					break;
				}
				default:
					ADRIA_ASSERT(false);
				}
			}
			else if (current_sky_type == 2)
			{
				renderer_settings.sky_type = ESkyType::HosekWilkie;
				ImGui::SliderFloat("Turbidity", &renderer_settings.turbidity, 2.0f, 30.0f);
				ImGui::SliderFloat("Ground Albedo", &renderer_settings.ground_albedo, 0.0f, 1.0f);
			}

        }
        ImGui::End();
	}

	void Editor::ParticleSettings()
	{
		ImGui::Begin("Particles");
        {
            static emitter_parameters_t params{};
            static char NAME_BUFFER[128];
            ImGui::InputText("Name", NAME_BUFFER, sizeof(NAME_BUFFER));
            params.name = std::string(NAME_BUFFER);
			if (ImGui::Button("Select Texture")) ImGuiFileDialog::Instance()->OpenDialog("Choose Texture", "Choose File", ".jpg,.jpeg,.tga,.dds,.png", ".");
			if (ImGuiFileDialog::Instance()->Display("Choose Texture"))
			{
				if (ImGuiFileDialog::Instance()->IsOk())
				{
					std::string texture_path = ImGuiFileDialog::Instance()->GetFilePathName();
                    params.texture_path = texture_path;
				}
				ImGuiFileDialog::Instance()->Close();
			}
            ImGui::Text(params.texture_path.c_str());

			ImGui::SliderFloat3("Position", params.position, -500.0f, 500.0f);
			ImGui::SliderFloat3("Velocity", params.velocity, -50.0f, 50.0f);
			ImGui::SliderFloat3("Position Variance", params.position_variance, -50.0f, 50.0f);
			ImGui::SliderFloat("Velocity Variance", &params.velocity_variance, -10.0f, 10.0f);
			ImGui::SliderFloat("Lifespan", &params.lifespan, 0.0f, 50.0f);
			ImGui::SliderFloat("Start Size", &params.start_size, 0.0f, 50.0f);
			ImGui::SliderFloat("End Size", &params.end_size, 0.0f, 10.0f);
			ImGui::SliderFloat("Mass", &params.mass, 0.0f, 10.0f);
			ImGui::SliderFloat("Particles Per Second", &params.particles_per_second, 1.0f, 1000.0f);
			ImGui::Checkbox("Alpha Blend", &params.blend);
			ImGui::Checkbox("Collisions", &params.collisions);
			ImGui::Checkbox("Sort", &params.sort);
            if (params.collisions) ImGui::SliderInt("Collision Thickness", &params.collision_thickness, 0, 40);

			if (ImGui::Button("Load Emitter"))
			{
				engine->entity_loader->LoadEmitter(params);
			}
        }
		ImGui::End();
	}

	void Editor::ListEntities()
    {
        auto all_entities = engine->reg.view<Tag>();

        ImGui::Begin("Entities");
        {
            if (ImGui::BeginPopupContextWindow(0, 1, false))
            {
                if (ImGui::MenuItem("Create"))
                {
                    entity empty = engine->reg.create();
                    engine->reg.emplace<Tag>(empty);
                }

                ImGui::EndPopup();
            }

            std::vector<entity> deleted_entities{};
            for (auto e : all_entities)
            {
                auto& tag = all_entities.get(e);

                ImGuiTreeNodeFlags flags = ((selected_entity == e) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
                flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
                bool opened = ImGui::TreeNodeEx(tag.name.c_str(), flags);

                
                if (ImGui::IsItemClicked())
                {
                    if (e == selected_entity)
                        selected_entity = null_entity;
                    else
                        selected_entity = e;
                }


                bool entity_deleted = false;
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Delete")) entity_deleted = true;

                    ImGui::EndPopup();
                }

                if (opened) ImGui::TreePop();

                if (entity_deleted)
                {
                    deleted_entities.push_back(e);
                    if (selected_entity == e) selected_entity = null_entity;
                }
            }

            for (auto e : deleted_entities)
                engine->reg.destroy(e);
        }
        ImGui::End();
    }

    void Editor::Properties()
    {
        ImGui::Begin("Properties");
        {
            if (selected_entity != null_entity)
            {
                auto tag = engine->reg.get_if<Tag>(selected_entity);
                if (tag)
                {
                    char buffer[256];
                    memset(buffer, 0, sizeof(buffer));
                    std::strncpy(buffer, tag->name.c_str(), sizeof(buffer));
                    if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
                        tag->name = std::string(buffer);
                }

                auto light = engine->reg.get_if<Light>(selected_entity);
                if (light && ImGui::CollapsingHeader("Light"))
                {

					if (light->type == ELightType::Directional)			ImGui::Text("Directional Light");
					else if (light->type == ELightType::Spot)			ImGui::Text("Spot Light");
					else if (light->type == ELightType::Point)			ImGui::Text("Point Light");

					XMFLOAT4 light_color, light_direction, light_position;
					XMStoreFloat4(&light_color, light->color);
					XMStoreFloat4(&light_direction, light->direction);
					XMStoreFloat4(&light_position, light->position);

					f32 color[3] = { light_color.x, light_color.y, light_color.z };

					ImGui::ColorEdit3("Light Color", color);

					light->color = XMVectorSet(color[0], color[1], color[2], 1.0f);

					ImGui::SliderFloat("Light Energy", &light->energy, 0.0f, 50.0f);

					if (engine->reg.has<Material>(selected_entity))
					{
						auto& material = engine->reg.get<Material>(selected_entity);
						material.diffuse = XMFLOAT3(color[0], color[1], color[2]);
					}

					if (light->type == ELightType::Directional || light->type == ELightType::Spot)
					{
						f32 direction[3] = { light_direction.x, light_direction.y, light_direction.z };

						ImGui::SliderFloat3("Light direction", direction, -1.0f, 1.0f);

						light->direction = XMVectorSet(direction[0], direction[1], direction[2], 0.0f);

						if (light->type == ELightType::Directional)
						{
							light->position = XMVectorScale(-light->direction, 1e3);
						}
					}

					if (light->type == ELightType::Spot)
					{
						f32 inner_angle = XMConvertToDegrees(acos(light->inner_cosine))
							, outer_angle = XMConvertToDegrees(acos(light->outer_cosine));
						ImGui::SliderFloat("Inner Spot Angle", &inner_angle, 0.0f, 90.0f);
						ImGui::SliderFloat("Outer Spot Angle", &outer_angle, inner_angle, 90.0f);

						light->inner_cosine = cos(XMConvertToRadians(inner_angle));
						light->outer_cosine = cos(XMConvertToRadians(outer_angle));
					}

					if (light->type == ELightType::Point || light->type == ELightType::Spot)
					{
						f32 position[3] = { light_position.x, light_position.y, light_position.z };

						ImGui::SliderFloat3("Light position", position, -300.0f, 500.0f);

						light->position = XMVectorSet(position[0], position[1], position[2], 1.0f);

						ImGui::SliderFloat("Range", &light->range, 50.0f, 1000.0f);
					}

					if (engine->reg.has<Transform>(selected_entity))
					{
						auto& tr = engine->reg.get<Transform>(selected_entity);

						tr.current_transform = XMMatrixTranslationFromVector(light->position);
					}

					ImGui::Checkbox("Active", &light->active);
					ImGui::Checkbox("God Rays", &light->god_rays);

					if (light->god_rays)
					{
						ImGui::SliderFloat("God Rays decay", &light->godrays_decay, 0.0f, 1.0f);
						ImGui::SliderFloat("God Rays weight", &light->godrays_weight, 0.0f, 0.5f);
						ImGui::SliderFloat("God Rays density", &light->godrays_density, 0.1f, 3.0f);
						ImGui::SliderFloat("God Rays exposure", &light->godrays_exposure, 0.1f, 10.0f);
					}


					ImGui::Checkbox("Casts Shadows", &light->casts_shadows);
					ImGui::Checkbox("Screen Space Shadows", &light->screen_space_shadows);
					ImGui::Checkbox("Volumetric Lighting", &light->volumetric);

					if (light->volumetric)
					{
						ImGui::SliderFloat("Volumetric lighting Strength", &light->volumetric_strength, 0.0f, 5.0f);
					}

					ImGui::Checkbox("Lens Flare", &light->lens_flare);

					if (light->type == ELightType::Directional && light->casts_shadows)
					{
						bool use_cascades = static_cast<bool>(light->use_cascades);
						ImGui::Checkbox("Use Cascades", &use_cascades);
						light->use_cascades = use_cascades;
					}

                }

                auto material = engine->reg.get_if<Material>(selected_entity);
                if (material && ImGui::CollapsingHeader("Material"))
                {
					ImGui::Text("Albedo Texture");
					ImGui::Image(engine->renderer->GetTextureManager().GetTextureView(material->albedo_texture),
						ImVec2(48.0f, 48.0f));

					ImGui::PushID(0);
					if (ImGui::Button("Remove")) material->albedo_texture = INVALID_TEXTURE_HANDLE;
					if (ImGui::Button("Select")) ImGuiFileDialog::Instance()->OpenDialog("Choose Texture", "Choose File", ".jpg,.jpeg,.tga,.dds,.png", ".");
					OpenMaterialFileDialog(material, EMaterialTextureType::Albedo);
					ImGui::PopID();

					ImGui::Text("Metallic-Roughness Texture");
					ImGui::Image(engine->renderer->GetTextureManager().GetTextureView(material->metallic_roughness_texture),
						ImVec2(48.0f, 48.0f));

					ImGui::PushID(1);
					if (ImGui::Button("Remove")) material->metallic_roughness_texture = INVALID_TEXTURE_HANDLE;
					if (ImGui::Button("Select")) ImGuiFileDialog::Instance()->OpenDialog("Choose Texture", "Choose File", ".jpg,.jpeg,.tga,.dds,.png", ".");
					OpenMaterialFileDialog(material, EMaterialTextureType::MetallicRoughness);
					ImGui::PopID();

					ImGui::Text("Emissive Texture");
					ImGui::Image(engine->renderer->GetTextureManager().GetTextureView(material->emissive_texture),
						ImVec2(48.0f, 48.0f));

					ImGui::PushID(2);
					if (ImGui::Button("Remove")) material->emissive_texture = INVALID_TEXTURE_HANDLE;
					if (ImGui::Button("Select")) ImGuiFileDialog::Instance()->OpenDialog("Choose Texture", "Choose File", ".jpg,.jpeg,.tga,.dds,.png", ".");
					OpenMaterialFileDialog(material, EMaterialTextureType::Emissive);
					ImGui::PopID();

					ImGui::ColorEdit3("Albedo Color", &material->diffuse.x);
					ImGui::SliderFloat("Albedo Factor", &material->albedo_factor, 0.0f, 1.0f);
					ImGui::SliderFloat("Metallic Factor", &material->metallic_factor, 0.0f, 1.0f);
					ImGui::SliderFloat("Roughness Factor", &material->roughness_factor, 0.0f, 1.0f);
					ImGui::SliderFloat("Emissive Factor", &material->emissive_factor, 0.0f, 32.0f);

					//add shader changing
					if (engine->reg.has<Forward>(selected_entity))
					{
						if (material->albedo_texture != INVALID_TEXTURE_HANDLE)
							material->shader = EShader::Texture;
						else material->shader = EShader::Solid;
					}
					else
					{
						material->shader = EShader::GBufferPBR;
					}
                }

                auto transform = engine->reg.get_if<Transform>(selected_entity);
                if (transform && ImGui::CollapsingHeader("Transform"))
                {
					XMFLOAT4X4 tr;
					XMStoreFloat4x4(&tr, transform->current_transform);

					float translation[3], rotation[3], scale[3];
					ImGuizmo::DecomposeMatrixToComponents(tr.m[0], translation, rotation, scale);
					ImGui::InputFloat3("Translation", translation);
					ImGui::InputFloat3("Rotation", rotation);
					ImGui::InputFloat3("Scale", scale);
					ImGuizmo::RecomposeMatrixFromComponents(translation, rotation, scale, tr.m[0]);

					if (Emitter* emitter = engine->reg.get_if<Emitter>(selected_entity))
					{
						emitter->position = XMFLOAT4(translation[0], translation[1], translation[2], 1.0f);
					}

					if (Visibility* vis = engine->reg.get_if<Visibility>(selected_entity))
					{
						vis->aabb.Transform(vis->aabb, DirectX::XMMatrixInverse(nullptr, transform->current_transform));
						transform->current_transform = DirectX::XMLoadFloat4x4(&tr);
						vis->aabb.Transform(vis->aabb, transform->current_transform);
					}
					else transform->current_transform = DirectX::XMLoadFloat4x4(&tr);

                }

                auto skybox = engine->reg.get_if<Skybox>(selected_entity);
                if (skybox && ImGui::CollapsingHeader("Skybox"))
                {
					ImGui::Checkbox("Active", &skybox->active);
					if (ImGui::Button("Change Skybox Texture")) ImGuiFileDialog::Instance()->OpenDialog("Choose Skybox Texture", "Choose File", ".hdr,.dds", ".");

					if (ImGuiFileDialog::Instance()->Display("Choose Skybox Texture"))
					{
						if (ImGuiFileDialog::Instance()->IsOk())
						{
							std::string texture_path = ImGuiFileDialog::Instance()->GetFilePathName();

							skybox->cubemap_texture = engine->renderer->GetTextureManager().LoadCubeMap(ConvertToWide(texture_path));
						}
						ImGuiFileDialog::Instance()->Close();
					}
                }

                auto forward = engine->reg.get_if<Forward>(selected_entity);
                if (forward)
                {
                    if (ImGui::CollapsingHeader("Forward")) ImGui::Checkbox("Transparent", &forward->transparent);
                }

                auto emitter = engine->reg.get_if<Emitter>(selected_entity);
                if (emitter && ImGui::CollapsingHeader("Emitter"))
                {
					ImGui::Text("Particle Texture");
					ImGui::Image(engine->renderer->GetTextureManager().GetTextureView(emitter->particle_texture),
						ImVec2(48.0f, 48.0f));

					ImGui::PushID(3);
					if (ImGui::Button("Remove")) emitter->particle_texture = INVALID_TEXTURE_HANDLE;
					if (ImGui::Button("Select")) ImGuiFileDialog::Instance()->OpenDialog("Choose Texture", "Choose File", ".jpg,.jpeg,.tga,.dds,.png", ".");
					if (ImGuiFileDialog::Instance()->Display("Choose Texture"))
					{
						if (ImGuiFileDialog::Instance()->IsOk())
						{
							std::string texture_path = ImGuiFileDialog::Instance()->GetFilePathName();
							emitter->particle_texture = engine->renderer->GetTextureManager().LoadTexture(texture_path);
						}
						ImGuiFileDialog::Instance()->Close();
					}
					ImGui::PopID();

					f32 pos[3] = { emitter->position.x, emitter->position.y, emitter->position.z },
						vel[3] = { emitter->velocity.x, emitter->velocity.y, emitter->velocity.z },
						pos_var[3] = { emitter->position_variance.x, emitter->position_variance.y, emitter->position_variance.z };

					ImGui::SliderFloat3("Position", pos, -500.0f, 500.0f);
					ImGui::SliderFloat3("Velocity", vel, -50.0f, 50.0f);
					ImGui::SliderFloat3("Position Variance", pos_var, -50.0f, 50.0f);
					emitter->position = DirectX::XMFLOAT4(pos[0], pos[1], pos[2], 1.0f);
					emitter->velocity = DirectX::XMFLOAT4(vel[0], vel[1], vel[2], 1.0f);
					emitter->position_variance = DirectX::XMFLOAT4(pos_var[0], pos_var[1], pos_var[2], 1.0f);

					if (transform)
					{
						XMFLOAT4X4 tr;
						XMStoreFloat4x4(&tr, transform->current_transform);
						f32 translation[3], rotation[3], scale[3];
						ImGuizmo::DecomposeMatrixToComponents(tr.m[0], translation, rotation, scale);
						ImGuizmo::RecomposeMatrixFromComponents(pos, rotation, scale, tr.m[0]);
						transform->current_transform = DirectX::XMLoadFloat4x4(&tr);
					}

					ImGui::SliderFloat("Velocity Variance", &emitter->velocity_variance, -10.0f, 10.0f);
					ImGui::SliderFloat("Lifespan", &emitter->particle_lifespan, 0.0f, 50.0f);
					ImGui::SliderFloat("Start Size", &emitter->start_size, 0.0f, 50.0f);
					ImGui::SliderFloat("End Size", &emitter->end_size, 0.0f, 10.0f);
					ImGui::SliderFloat("Mass", &emitter->mass, 0.0f, 10.0f);
					ImGui::SliderFloat("Particles Per Second", &emitter->particles_per_second, 1.0f, 1000.0f);

					ImGui::Checkbox("Alpha Blend", &emitter->alpha_blended);
					ImGui::Checkbox("Collisions", &emitter->collisions_enabled);
					if (emitter->collisions_enabled) ImGui::SliderInt("Collision Thickness", &emitter->collision_thickness, 0, 40);

					ImGui::Checkbox("Sort", &emitter->sort);
					ImGui::Checkbox("Pause", &emitter->pause);
					if (ImGui::Button("Reset")) emitter->reset_emitter = true;
                }

                static char const* const components[] = { "Mesh", "Transform", "Material",
               "Visibility", "Light", "Skybox", "Deferred", "Forward", "Emitter"};
                
                static int current_component = 0;
                char const* combo_label = components[current_component];
                if (ImGui::BeginCombo("Components", combo_label, 0))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(components); n++)
                    {
                        const bool is_selected = (current_component == n);
                        if (ImGui::Selectable(components[n], is_selected))
                            current_component = n;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                enum COMPONENT_INDEX
                {
                    MESH,
                    TRANSFORM,
                    MATERIAL,
                    VISIBILITY,
                    LIGHT,
                    SKYBOX,
                    DEFERRED,
                    FORWARD,
                    EMITTER
                };

                static model_parameters_t params{};
                if (current_component == MESH)
                {
                    
                    if(ImGui::Button("Choose Mesh"))
                        ImGuiFileDialog::Instance()->OpenDialog("Choose Mesh", "Choose File", ".obj,.gltf", ".");

                    if (ImGuiFileDialog::Instance()->Display("Choose Mesh"))
                    {

                        if (ImGuiFileDialog::Instance()->IsOk())
                        {
                            std::string model_path = ImGuiFileDialog::Instance()->GetFilePathName();
                            params.model_path = model_path;
                        }

                    ImGuiFileDialog::Instance()->Close();
                    }
                }

                static ELightType light_type = ELightType::Point;
                if (current_component == LIGHT)
                {
                    static char const* const light_types[] = { "Directional", "Point", "Spot"};

                    static int current_light_type = 0;
                    ImGui::ListBox("Light Types", &current_light_type, light_types, IM_ARRAYSIZE(light_types));
                    light_type = static_cast<ELightType>(current_light_type);
                }

                if (ImGui::Button("Add Component"))
                {
                    switch(current_component)
                    {
                    case MESH:
                        if (engine->reg.has<Mesh>(selected_entity))
                        {
                            ADRIA_LOG(WARNING, "Entity already has Mesh Component!");
                        }
                        else
                        {
                            ADRIA_LOG(WARNING, "Not supported for now!");
                        }
                        break;
                    case TRANSFORM:
                        if (engine->reg.has<Transform>(selected_entity))
                        {
                            ADRIA_LOG(WARNING, "Entity already has Transform Component!");
                        }
                        else engine->reg.emplace<Transform>(selected_entity);
                        break;
                    case MATERIAL:
                        if (engine->reg.has<Material>(selected_entity))
                        {
                            ADRIA_LOG(WARNING, "Entity already has Material Component!");
                        }
                        else
                        {
                            Material mat{};
                            if (engine->reg.has<Deferred>(selected_entity))
                                mat.shader = EShader::GBufferPBR;
                            else mat.shader = EShader::Solid;
                            engine->reg.emplace<Material>(selected_entity, mat);
                        }
                        break;
                    case VISIBILITY:
                        if (engine->reg.has<Visibility>(selected_entity))
						{
							ADRIA_LOG(WARNING, "Entity already has Visibility Component!");
						}
                        else if (!engine->reg.has<Mesh>(selected_entity) || !engine->reg.has<Transform>(selected_entity))
                        {
                            ADRIA_LOG(WARNING, "Entity has to have Mesh and Transform Component before adding Visibility!");
                        }
                        break;
                    case LIGHT:
                        if (engine->reg.has<Light>(selected_entity))
						{
							ADRIA_LOG(WARNING, "Entity already has Light Component!");
						}
                        else
                        {
                            Light light{};
                            light.type = light_type;
                            engine->reg.emplace<Light>(selected_entity, light);
                        }
                        break;
                    case SKYBOX:
                        if (engine->reg.has<Skybox>(selected_entity))
						{
							ADRIA_LOG(WARNING, "Entity already has Skybox Component!");
						}
                        else 
                        {
                            engine->reg.emplace<Skybox>(selected_entity);
                        }
                        break;
                    case DEFERRED:
                        if(engine->reg.has<Deferred>(selected_entity))
						{
							ADRIA_LOG(WARNING, "Entity already has Deferred Component!");
						}
                        else if (engine->reg.has<Forward>(selected_entity))
                        {
                            ADRIA_LOG(WARNING, "Cannot add Deferred Component to entity that has Forward Component!");
                        }
                        else 
                        {
							engine->reg.emplace<Deferred>(selected_entity);
                        }
                        break;
                    case FORWARD:
                        if (engine->reg.has<Forward>(selected_entity))
						{
							ADRIA_LOG(WARNING, "Entity already has Forward Component!");
						}
                        else if (engine->reg.has<Deferred>(selected_entity))
                        {
                            ADRIA_LOG(WARNING, "Cannot add Forward Component to entity that has Deferred Component!");
                        }
                        else 
                        {
                            engine->reg.emplace<Forward>(selected_entity, false);
                        }
                        break;
                    case EMITTER:
                        if (engine->reg.has<Emitter>(selected_entity))
                        {
                            ADRIA_LOG(WARNING, "Entity already has Emitter Component!");
                        }
                        else
                        {
                            engine->reg.emplace<Emitter>(selected_entity);
                        }
                    }
                }

                if (ImGui::Button("Remove Component"))
                {
                    switch (current_component)
                    {
                    case MESH:
                        if (!engine->reg.has<Mesh>(selected_entity))
                        {
                            ADRIA_LOG(WARNING, "Entity doesn't have Mesh Component!");
                        }
                        else 
                        {
                            engine->reg.remove<Mesh>(selected_entity);
                        }
                        break;
                    case TRANSFORM:
						if (!engine->reg.has<Transform>(selected_entity))
						{
							ADRIA_LOG(WARNING, "Entity doesn't have Transform Component!");
						}
						else
						{
							engine->reg.remove<Transform>(selected_entity);
						}
                        break;
                    case MATERIAL:
						if (!engine->reg.has<Material>(selected_entity))
						{
							ADRIA_LOG(WARNING, "Entity doesn't have Material Component!");
						}
						else
						{
							engine->reg.remove<Material>(selected_entity);
						}
                        break;
                    case VISIBILITY:
						if (!engine->reg.has<Visibility>(selected_entity))
						{
							ADRIA_LOG(WARNING, "Entity doesn't have Visibility Component!");
						}
						else
						{
							engine->reg.remove<Visibility>(selected_entity);
						}
                        break;
                    case LIGHT:
						if (!engine->reg.has<Light>(selected_entity))
						{
							ADRIA_LOG(WARNING, "Entity doesn't have Light Component!");
						}
						else
						{
							engine->reg.remove<Light>(selected_entity);
						}
                        break;
                    case SKYBOX:
						if (!engine->reg.has<Skybox>(selected_entity))
						{
							ADRIA_LOG(WARNING, "Entity doesn't have Skybox Component!");
						}
						else
						{
							engine->reg.remove<Skybox>(selected_entity);
						}
                        break;
                    case DEFERRED:
						if (!engine->reg.has<Deferred>(selected_entity))
						{
							ADRIA_LOG(WARNING, "Entity doesn't have Deferred Component!");
						}
						else
						{
							engine->reg.remove<Deferred>(selected_entity);
						}
                        break;
                    case FORWARD:
						if (!engine->reg.has<Forward>(selected_entity))
						{
							ADRIA_LOG(WARNING, "Entity doesn't have Forward Component!");
						}
						else
						{
							engine->reg.remove<Forward>(selected_entity);
						}
                        break;
                    }
                }
            }
        }
        ImGui::End();
    }

    void Editor::Camera()
    {
        auto& camera = engine->camera_manager.GetActiveCamera();

        static bool is_open = true;
		if (is_open)
		{
			if (ImGui::Begin("Camera", &is_open))
			{
				f32 pos[3] = { camera.Position().m128_f32[0],camera.Position().m128_f32[1], camera.Position().m128_f32[2] };
				ImGui::SliderFloat3("Position", pos, 0.0f, 10000.0f);
				camera.SetPosition(DirectX::XMFLOAT3(pos));
				f32 _near = camera.Near(), _far = camera.Far();
				f32 _fov = camera.Fov(), _ar = camera.AspectRatio();
				ImGui::SliderFloat("Near Plane", &_near, 0.0f, 2.0f);
				ImGui::SliderFloat("Far Plane", &_far, 10.0f, 5000.0f);
				ImGui::SliderFloat("FOV", &_fov, 0.01f, 1.5707f);
				camera.SetNearAndFar(_near, _far);
				camera.SetFov(_fov);
			}
            ImGui::End();
		}
    }

    void Editor::Scene()
    {
        ImGui::Begin("Scene");
        {
            ImVec2 _scene_dimension = ImGui::GetWindowSize();
            ImGui::Image(engine->renderer->GetOffscreenTexture().SRV(), _scene_dimension);

            mouse_in_scene = ImGui::IsWindowFocused();
        }

        if (selected_entity != null_entity && engine->reg.has<Transform>(selected_entity) && gizmo_enabled)
        {

            ImGuizmo::SetDrawlist();

            ImVec2 window_size = ImGui::GetWindowSize();
            ImVec2 window_pos = ImGui::GetWindowPos();

            ImGuizmo::SetRect(window_pos.x, window_pos.y,
                window_size.x, window_size.y);

            auto& camera = engine->camera_manager.GetActiveCamera();

            auto camera_view = camera.View();
            auto camera_proj = camera.Proj();

            DirectX::XMFLOAT4X4 view, projection;

            DirectX::XMStoreFloat4x4(&view, camera_view);
            DirectX::XMStoreFloat4x4(&projection, camera_proj);


            auto& entity_transform = engine->reg.get<Transform>(selected_entity);

            DirectX::XMFLOAT4X4 tr;
            DirectX::XMStoreFloat4x4(&tr, entity_transform.current_transform);

            ImGuizmo::Manipulate(view.m[0], projection.m[0], gizmo_op, ImGuizmo::LOCAL,
                tr.m[0]);

            if (ImGuizmo::IsUsing())
            {
                Visibility* vis = engine->reg.get_if<Visibility>(selected_entity);

                if (vis)
                {
                    vis->aabb.Transform(vis->aabb, DirectX::XMMatrixInverse(nullptr, entity_transform.current_transform));
                    entity_transform.current_transform = DirectX::XMLoadFloat4x4(&tr);
                    vis->aabb.Transform(vis->aabb, entity_transform.current_transform);
                }
                else entity_transform.current_transform = DirectX::XMLoadFloat4x4(&tr);

            }
        }

        ImGui::End();

    }

    void Editor::Log()
    {
        ImGui::Begin("Log");
        {
            editor_log->Draw("Log");
        }
        ImGui::End();
    }

    void Editor::RendererSettings()
    {  
        ImGui::Begin("Renderer Settings");
        {
            if (ImGui::TreeNode("Deferred Settings"))
            {
                const char* deferred_types[] = { "Regular", "Tiled", "Clustered" };
                static int current_deferred_type = 0;
                const char* combo_label = deferred_types[current_deferred_type];
                if (ImGui::BeginCombo("Deferred Type", combo_label, 0))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(deferred_types); n++)
                    {
                        const bool is_selected = (current_deferred_type == n);
                        if (ImGui::Selectable(deferred_types[n], is_selected)) current_deferred_type = n;
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                renderer_settings.use_tiled_deferred = (current_deferred_type == 1);
                renderer_settings.use_clustered_deferred = (current_deferred_type == 2);

                if (renderer_settings.use_tiled_deferred && ImGui::TreeNodeEx("Tiled Deferred", ImGuiTreeNodeFlags_OpenOnDoubleClick))
                {
                    ImGui::Checkbox("Visualize Tiles", &renderer_settings.visualize_tiled);
                    if (renderer_settings.visualize_tiled) ImGui::SliderInt("Visualize Scale", &renderer_settings.visualize_max_lights, 1, 32);

                    ImGui::TreePop();
                    ImGui::Separator();
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Global Illumination"))
            {

                ImGui::Checkbox("Voxel Cone Tracing GI", &renderer_settings.voxel_gi);

                if (renderer_settings.voxel_gi)
                {
                    bool voxel_size_changed = ImGui::SliderFloat("Voxel Size", &renderer_settings.voxel_size, 0.125f, 8.0f);
                    ImGui::SliderInt("Cone Number", &renderer_settings.voxel_num_cones, 1, 6);
                    ImGui::SliderFloat("Ray Step", &renderer_settings.voxel_ray_step_distance, 0.5f, 5.0f);
                    ImGui::SliderFloat("Max Distance", &renderer_settings.voxel_max_distance, 1.0f, 50.0f);
                    ImGui::Checkbox("Second Bounce", &renderer_settings.voxel_second_bounce);
                    ImGui::Checkbox("Voxel GI Debug", &renderer_settings.voxel_debug);
                }
                renderer_settings.voxel_debug = renderer_settings.voxel_debug && renderer_settings.voxel_gi; //voxel debug cannot be true unless voxel_gi is true

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Postprocessing"))
            {
                //ambient oclussion
                {
                    const char* ao_types[] = { "None", "SSAO", "HBAO" };
                    static int current_ao_type = 0;
                    const char* combo_label = ao_types[current_ao_type];
                    if (ImGui::BeginCombo("Ambient Occlusion", combo_label, 0))
                    {
                        for (int n = 0; n < IM_ARRAYSIZE(ao_types); n++)
                        {
                            const bool is_selected = (current_ao_type == n);
                            if (ImGui::Selectable(ao_types[n], is_selected)) current_ao_type = n;
                            if (is_selected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    renderer_settings.ambient_occlusion = static_cast<EAmbientOcclusion>(current_ao_type);

                    if (renderer_settings.ambient_occlusion == EAmbientOcclusion::SSAO && ImGui::TreeNodeEx("SSAO", ImGuiTreeNodeFlags_OpenOnDoubleClick))
                    {
                        //ImGui::Checkbox("SSAO", &settings.ssao);
                        ImGui::SliderFloat("Power", &renderer_settings.ssao_power, 1.0f, 16.0f);
                        ImGui::SliderFloat("Radius", &renderer_settings.ssao_radius, 0.5f, 4.0f);

                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                    if (renderer_settings.ambient_occlusion == EAmbientOcclusion::HBAO && ImGui::TreeNodeEx("HBAO", ImGuiTreeNodeFlags_OpenOnDoubleClick))
                    {
                        //ImGui::Checkbox("SSAO", &settings.ssao);
                        ImGui::SliderFloat("Power", &renderer_settings.hbao_power, 1.0f, 16.0f);
                        ImGui::SliderFloat("Radius", &renderer_settings.hbao_radius, 0.25f, 8.0f);

                        ImGui::TreePop();
                        ImGui::Separator();
                    }
                }

                ImGui::Checkbox("Volumetric Clouds", &renderer_settings.clouds);
                ImGui::Checkbox("SSR", &renderer_settings.ssr);
                ImGui::Checkbox("DoF", &renderer_settings.dof);
                ImGui::Checkbox("Bloom", &renderer_settings.bloom);
                ImGui::Checkbox("Motion Blur", &renderer_settings.motion_blur);
                ImGui::Checkbox("Fog", &renderer_settings.fog);
                if (ImGui::TreeNode("Anti-Aliasing"))
                {
                    static bool fxaa = false, taa = false;
                    ImGui::Checkbox("FXAA", &fxaa);
                    ImGui::Checkbox("TAA", &taa);
                    if (fxaa)
                    {
                        renderer_settings.anti_aliasing = static_cast<EAntiAliasing>(renderer_settings.anti_aliasing | EAntiAliasing_FXAA);
                    }
                    else
                    {
                        renderer_settings.anti_aliasing = static_cast<EAntiAliasing>(renderer_settings.anti_aliasing & (~EAntiAliasing_FXAA));
                    }
                    if (taa)
                    {
                        renderer_settings.anti_aliasing = static_cast<EAntiAliasing>(renderer_settings.anti_aliasing | EAntiAliasing_TAA);
                    }
                    else
                    {
                        renderer_settings.anti_aliasing = static_cast<EAntiAliasing>(renderer_settings.anti_aliasing & (~EAntiAliasing_TAA));
                    }
                    
                    ImGui::TreePop();
                }
                if (renderer_settings.clouds && ImGui::TreeNodeEx("Volumetric Clouds", 0))
                {
                    ImGui::SliderFloat("Sun light absorption", &renderer_settings.light_absorption, 0.0f, 0.015f);
                    ImGui::SliderFloat("Clouds bottom height", &renderer_settings.clouds_bottom_height, 1000.0f, 10000.0f);
                    ImGui::SliderFloat("Clouds top height", &renderer_settings.clouds_top_height, 10000.0f, 50000.0f);
                    ImGui::SliderFloat("Density", &renderer_settings.density_factor, 0.0f, 1.0f);
                    ImGui::SliderFloat("Crispiness", &renderer_settings.crispiness, 0.0f, 100.0f);
                    ImGui::SliderFloat("Curliness", &renderer_settings.curliness, 0.0f, 5.0f);
                    ImGui::SliderFloat("Coverage", &renderer_settings.coverage, 0.0f, 1.0f);
                    ImGui::SliderFloat("Cloud Type", &renderer_settings.cloud_type, 0.0f, 1.0f);
                    ImGui::TreePop();
                    ImGui::Separator();
                }
                if (renderer_settings.ssr && ImGui::TreeNodeEx("Screen-Space Reflections", 0))
                {
                    ImGui::SliderFloat("Ray Step", &renderer_settings.ssr_ray_step, 1.0f, 3.0f);
                    ImGui::SliderFloat("Ray Hit Threshold", &renderer_settings.ssr_ray_hit_threshold, 0.25f, 5.0f);

                    ImGui::TreePop();
                    ImGui::Separator();
                }
                if (renderer_settings.dof && ImGui::TreeNodeEx("Depth Of Field", 0))
                {

                    ImGui::SliderFloat("DoF Near Blur", &renderer_settings.dof_near_blur, 0.0f, 200.0f);
                    ImGui::SliderFloat("DoF Near", &renderer_settings.dof_near, renderer_settings.dof_near_blur, 500.0f);
                    ImGui::SliderFloat("DoF Far", &renderer_settings.dof_far, renderer_settings.dof_near, 1000.0f);
                    ImGui::SliderFloat("DoF Far Blur", &renderer_settings.dof_far_blur, renderer_settings.dof_far, 1500.0f);
                    ImGui::Checkbox("Bokeh", &renderer_settings.bokeh);

                    if (renderer_settings.bokeh)
                    {
                        static char const* const bokeh_types[] = { "HEXAGON", "OCTAGON", "CIRCLE", "CROSS" };
                        static int bokeh_type_i = static_cast<int>(renderer_settings.bokeh_type);
                        ImGui::ListBox("Bokeh Type", &bokeh_type_i, bokeh_types, IM_ARRAYSIZE(bokeh_types));
                        renderer_settings.bokeh_type = static_cast<EBokehType>(bokeh_type_i);

                        ImGui::SliderFloat("Bokeh Blur Threshold", &renderer_settings.bokeh_blur_threshold, 0.0f, 1.0f);
                        ImGui::SliderFloat("Bokeh Lum Threshold", &renderer_settings.bokeh_lum_threshold, 0.0f, 10.0f);
                        ImGui::SliderFloat("Bokeh Color Scale", &renderer_settings.bokeh_color_scale, 0.1f, 10.0f);
                        ImGui::SliderFloat("Bokeh Max Size", &renderer_settings.bokeh_radius_scale, 0.0f, 100.0f);
                        ImGui::SliderFloat("Bokeh Fallout", &renderer_settings.bokeh_fallout, 0.0f, 2.0f);
                    }
                    ImGui::TreePop();
                    ImGui::Separator();

                }
                if (renderer_settings.bloom && ImGui::TreeNodeEx("Bloom", 0))
                {
                    ImGui::SliderFloat("Threshold", &renderer_settings.bloom_threshold, 0.1f, 2.0f);
                    ImGui::SliderFloat("Bloom Scale", &renderer_settings.bloom_scale, 0.1f, 5.0f);
                    ImGui::TreePop();
                    ImGui::Separator();
                }
                if ((renderer_settings.motion_blur || (renderer_settings.anti_aliasing & EAntiAliasing_TAA)) && ImGui::TreeNodeEx("Velocity Buffer", 0))
                {
                    ImGui::SliderFloat("Motion Blur Scale", &renderer_settings.velocity_buffer_scale, 32.0f, 128.0f);
                    ImGui::TreePop();
                    ImGui::Separator();
                }
                if (renderer_settings.fog && ImGui::TreeNodeEx("Fog", 0))
                {
                    const char* fog_types[] = { "Exponential", "Exponential Height"};
                    static int current_fog_type = 0; // Here we store our selection data as an index.
                    const char* combo_label = fog_types[current_fog_type];  // Label to preview before opening the combo (technically it could be anything)
                    if (ImGui::BeginCombo("Fog Type", combo_label, 0))
                    {
                        for (int n = 0; n < IM_ARRAYSIZE(fog_types); n++)
                        {
                            const bool is_selected = (current_fog_type == n);
                            if (ImGui::Selectable(fog_types[n], is_selected)) current_fog_type = n;
                            if (is_selected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    
                    renderer_settings.fog_type = static_cast<EFogType>(current_fog_type);

                    ImGui::SliderFloat("Fog Falloff", &renderer_settings.fog_falloff, 0.0001f, 0.01f);
                    ImGui::SliderFloat("Fog Density", &renderer_settings.fog_density, 0.0001f, 0.01f);
                    ImGui::SliderFloat("Fog Start", &renderer_settings.fog_start, 0.1f, 10000.0f);
                    ImGui::ColorEdit3("Fog Color", renderer_settings.fog_color);

                    ImGui::TreePop();
                    ImGui::Separator();
                }
                if (ImGui::TreeNodeEx("Tone Mapping", 0))
                {
                    ImGui::SliderFloat("Exposure", &renderer_settings.tone_map_exposure, 0.01f, 10.0f);
                    static char const* const operators[] = { "REINHARD", "HABLE", "LINEAR" };
                    static int tone_map_operator = static_cast<int>(renderer_settings.tone_map_op);
                    ImGui::ListBox("Tone Map Operator", &tone_map_operator, operators, IM_ARRAYSIZE(operators));
                    renderer_settings.tone_map_op = static_cast<EToneMap>(tone_map_operator);
                    ImGui::TreePop();
                    ImGui::Separator();
                }

                ImGui::TreePop();
            }
            
            if (ImGui::TreeNode("Misc"))
            {

				renderer_settings.recreate_initial_spectrum = ImGui::SliderFloat2("Wind Direction", renderer_settings.wind_direction, 0.0f, 50.0f);
				ImGui::SliderFloat("Wind speed factor", &renderer_settings.wind_speed, 0.0f, 100.0f);
                ImGui::ColorEdit3("Ambient Color", renderer_settings.ambient_color);
                ImGui::SliderFloat("Shadow Softness", &renderer_settings.shadow_softness, 0.01f, 5.0f);
                ImGui::Checkbox("Transparent Shadows", &renderer_settings.shadow_transparent);
                ImGui::Checkbox("IBL", &renderer_settings.ibl);

                //random lights
                {
                    ImGui::Text("For Easy Demonstration of Tiled/Clustered Deferred Rendering");
                    static int light_count_to_add = 1;
                    ImGui::SliderInt("Light Count", &light_count_to_add, 1, 128);

                    if (ImGui::Button("Random Point Lights"))
                    {
                        static RealRandomGenerator real(0.0f, 1.0f);

                        for (i32 i = 0; i < light_count_to_add; ++i)
                        {
                            light_parameters_t light_params{};
                            light_params.light_data.casts_shadows = false;
                            light_params.light_data.color = DirectX::XMVectorSet(real() * 2, real() * 2, real() * 2, 1.0f);
                            light_params.light_data.direction = DirectX::XMVectorSet(0.5f, -1.0f, 0.1f, 0.0f);
                            light_params.light_data.position = DirectX::XMVectorSet(real() * 500 - 250, real() * 500.0f, real() * 500 - 250, 1.0f);
                            light_params.light_data.type = ELightType::Point;
                            light_params.mesh_type = ELightMesh::NoMesh;
                            light_params.light_data.range = real() * 100.0f + 40.0f;
                            light_params.light_data.active = true;
                            light_params.light_data.volumetric = false;
                            light_params.light_data.volumetric_strength = 1.0f;
                            engine->entity_loader->LoadLight(light_params);
                        }
                    }

                }

                ImGui::TreePop();
            }
        }
        ImGui::End();
    }

    void Editor::StatsAndProfiling()
    {
        if (ImGui::Begin("Profiling"))
        {
            ImGuiIO io = ImGui::GetIO();
            ImGui::TextWrapped("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			static bool enable_profiling = false;
			ImGui::Checkbox("Enable Profiling", &enable_profiling);
			if (enable_profiling)
			{
                static bool log_results = false;
                ImGui::Checkbox("Log Results", &log_results);
				ImGui::Checkbox("Profile GBuffer Pass", &profiler_settings.profile_gbuffer_pass);
				ImGui::Checkbox("Profile Deferred Pass", &profiler_settings.profile_deferred_pass);
				ImGui::Checkbox("Profile Forward Pass", &profiler_settings.profile_forward_pass);
                ImGui::Checkbox("Profile Particles Pass", &profiler_settings.profile_particles_pass);
				ImGui::Checkbox("Profile Postprocessing", &profiler_settings.profile_postprocessing);
				
                engine->renderer->SetProfilerSettings(profiler_settings);

				std::vector<std::string> results = engine->renderer->GetProfilerResults(log_results);
				std::string concatenated_results;
				for (auto const& result : results) concatenated_results += result;
				ImGui::TextWrapped(concatenated_results.c_str());
			}
			else
			{
                engine->renderer->SetProfilerSettings(NO_PROFILING);
			}
        }
        ImGui::End();
    }

    void Editor::OpenMaterialFileDialog(Material* material, EMaterialTextureType type)
    {
        if (ImGuiFileDialog::Instance()->Display("Choose Texture"))
        {
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string texture_path = ImGuiFileDialog::Instance()->GetFilePathName();

                switch (type)
                {
                case EMaterialTextureType::Albedo:
                    material->albedo_texture = engine->renderer->GetTextureManager().LoadTexture(texture_path);
                    break;
                case EMaterialTextureType::MetallicRoughness:
                    material->metallic_roughness_texture = engine->renderer->GetTextureManager().LoadTexture(texture_path);
                    break;
                case EMaterialTextureType::Emissive:
                    material->emissive_texture = engine->renderer->GetTextureManager().LoadTexture(texture_path);
                    break;
                }
            }

            ImGuiFileDialog::Instance()->Close();
        }
    }
}
