#pragma once
#include <string>

namespace adria
{
	class GfxDevice;
	struct WindowEventData;

	class GUI
	{
	public:

		explicit GUI(GfxDevice* gfx);
		~GUI();
		void Begin() const;
		void End() const;
		void HandleWindowMessage(WindowEventData const&) const;
		void ToggleVisibility();
		bool IsVisible() const;

	private:
		std::string ini_file;
		bool visible = true;
	};
}