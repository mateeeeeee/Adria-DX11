#include "GUI.h"
#include "IconsFontAwesome4.h"
#include "Core/Window.h"
#include "Core/pATHS.h"
#include "Graphics/GfxDevice.h"
#include "ImGui/ImGuizmo.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace adria
{
	GUI::GUI(GfxDevice* gfx)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ini_file = paths::IniDir() + "imgui.ini";
		io.IniFilename = ini_file.c_str();

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigWindowsResizeFromEdges = true;
		io.ConfigViewportsNoTaskBarIcon = true;

		ImFontConfig font_config{};
		std::string font_path = paths::FontsDir() + "roboto/Roboto-Light.ttf";
		io.Fonts->AddFontFromFileTTF(font_path.c_str(), 16.0f, &font_config);
		font_config.MergeMode = true;
		ImWchar const icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		std::string icon_path = paths::FontsDir() + FONT_ICON_FILE_NAME_FA;
		io.Fonts->AddFontFromFileTTF(icon_path.c_str(), 15.0f, &font_config, icon_ranges);
		io.Fonts->Build();

		ImGui_ImplWin32_Init(gfx->GetWindow()->Handle());
		ImGui_ImplDX11_Init(gfx->GetDevice(), gfx->GetContext());

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowBorderSize = 1.0f;
		style.FrameBorderSize = 0.0f;
		style.ScrollbarSize = 20.0f;
		style.FramePadding = ImVec2(5, 5);
		style.ItemSpacing = ImVec2(6, 5);
		style.WindowMenuButtonPosition = ImGuiDir_Right;
		style.WindowRounding = 2.0f;
		style.FrameRounding = 2.0f;
		style.PopupRounding = 2.0f;
		style.GrabRounding = 2.0f;
		style.ScrollbarRounding = 2.0f;
		style.Alpha = 1.0f;

	}
	GUI::~GUI()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
	void GUI::Begin() const
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}
	void GUI::End() const
	{
		ImGui::Render();
		if (visible)
		{
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		}

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

	}
	void GUI::HandleWindowMessage(WindowEventData const& data) const
	{
		ImGui_ImplWin32_WndProcHandler(static_cast<HWND>(data.handle), data.msg, data.wparam, data.lparam);
	}
	void GUI::ToggleVisibility()
	{
		visible = !visible;
	}
	bool GUI::IsVisible() const
	{
		return visible;
	}
}