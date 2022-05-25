#pragma once
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"

namespace adria
{
	class GraphicsDeviceDX11;
	struct WindowMessage;

	class GUI
	{
	public:

		GUI(GraphicsDeviceDX11* gfx);

		~GUI();

		void Begin() const;

		void End() const;

		void HandleWindowMessage(WindowMessage const&) const;

		void ToggleVisibility();

		bool IsVisible() const;

	private:

		bool visible = true;
	};
}