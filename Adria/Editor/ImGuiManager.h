#pragma once
#include <string>

namespace adria
{
	class GfxDevice;
	struct WindowEventData;

	class ImGuiManager
	{
	public:

		explicit ImGuiManager(GfxDevice* gfx);
		~ImGuiManager();
		void Begin() const;
		void End() const;
		void HandleWindowMessage(WindowEventData const&) const;
		void ToggleVisibility();
		Bool IsVisible() const;

	private:
		std::string ini_file;
		Bool visible = true;
	};
}