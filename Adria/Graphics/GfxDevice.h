#pragma once
#include <d3d11_4.h>


namespace adria
{
	class Window;
	class GfxCommandContext;

	class GfxDevice
	{
	public:
		explicit GfxDevice(Window* window);
		GfxDevice(GfxDevice const&) = delete;
		GfxDevice(GfxDevice&&) = default;
		GfxDevice& operator=(GfxDevice const&) = delete;
		GfxDevice& operator=(GfxDevice&&) = default;
		~GfxDevice();

		void ResizeBackbuffer(Uint32 w, Uint32 h);
		void ClearBackbuffer();
		void SwapBuffers(Bool vsync);
		void SetBackbuffer();

		ID3D11Device3* GetDevice() const;
		ID3D11DeviceContext4* GetContext() const;
		GfxCommandContext* GetCommandContext() const { return command_context.get(); }
		Window* GetWindow() const { return window; }

	private:
		Window* window;
		Uint32 width, height;
		Ref<ID3D11Device3> device = nullptr;
		Ref<IDXGISwapChain> swapchain = nullptr;
		std::unique_ptr<GfxCommandContext> command_context;
		Ref<ID3D11Texture2D> backbuffer = nullptr;
		Ref<ID3D11RenderTargetView> backbuffer_rtv = nullptr;

	private:

		void WaitForGPU();
		void CreateBackBufferResources(Uint32 w, Uint32 h);
	};
}