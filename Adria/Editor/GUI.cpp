#include "GUI.h"
#include "../Core/Window.h"
#include "../Graphics/GraphicsDeviceDX11.h"
#include "../ImGui/ImGuizmo.h"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace adria
{
	GUI::GUI(GraphicsDeviceDX11* gfx)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.ConfigWindowsResizeFromEdges = true;
		io.ConfigViewportsNoTaskBarIcon = true;

		ImGui_ImplWin32_Init(Window::Handle());
		ImGui_ImplDX11_Init(gfx->Device(), gfx->Context());

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

		// ImGui - child windows
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

	}
	void GUI::HandleWindowMessage(WindowMessage const& msg_data) const
	{
		ImGui_ImplWin32_WndProcHandler(static_cast<HWND>(msg_data.handle),
			msg_data.msg, msg_data.wparam, msg_data.lparam);
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