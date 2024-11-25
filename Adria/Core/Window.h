#pragma once
#include "Windows.h"
#include "Utilities/Delegate.h"

namespace adria
{
	struct WindowEventData
	{
		void* handle = nullptr;
		Uint32 msg = 0;
		Uint64 wparam = 0;
		Int64 lparam = 0;
		Float width = 0.0f;
		Float height = 0.0f;
	};

	struct WindowInit
	{
		HINSTANCE instance;
		Char const* title;
		Uint32 width, height;
		Bool maximize;
	};

	DECLARE_EVENT(WindowEvent, Window, WindowEventData const&);

	class Window
	{
		friend LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	public:
		Window(WindowInit const& init);
		~Window();

		Uint32 Width() const;
		Uint32 Height() const;

		Uint32 PositionX() const;
		Uint32 PositionY() const;

		Bool Loop();
		void Quit(Int32 exit_code);

		void* Handle() const;
		Bool  IsActive() const;

		WindowEvent& GetWindowEvent() { return window_event; }

	private:
		HINSTANCE hinstance = nullptr;
		HWND hwnd = nullptr;
		WindowEvent window_event;

	private:
		void BroadcastEvent(WindowEventData const&);
	};
}