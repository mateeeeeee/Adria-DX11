#pragma once

namespace adria
{
	class GfxDevice;
	struct WindowMessage;

	class GUI
	{
	public:

		explicit GUI(GfxDevice* gfx);
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