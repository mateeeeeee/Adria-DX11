#pragma once
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "dxguid.lib")
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

		void ResizeBackbuffer(uint32 w, uint32 h);
		void ClearBackbuffer();
		void SwapBuffers(bool vsync);
		void SetBackbuffer();

		ID3D11Device3* GetDevice() const;
		ID3D11DeviceContext4* GetContext() const;
		GfxCommandContext* GetCommandContext() const { return command_context.get(); }
		Window* GetWindow() const { return window; }

	private:
		Window* window;
		uint32 width, height;
		ArcPtr<ID3D11Device3> device = nullptr;
		ArcPtr<IDXGISwapChain> swapchain = nullptr;
		std::unique_ptr<GfxCommandContext> command_context;
		ArcPtr<ID3D11Texture2D> backbuffer = nullptr;
		ArcPtr<ID3D11RenderTargetView> backbuffer_rtv = nullptr;

	private:

		void WaitForGPU();
		void CreateBackBufferResources(uint32 w, uint32 h);
	};
}