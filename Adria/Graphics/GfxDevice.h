#pragma once
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "dxguid.lib")

#include <d3d11_1.h>

namespace adria
{
	class GfxDevice
	{
	public:
		explicit GfxDevice(void* handle);
		GfxDevice(GfxDevice const&) = delete;
		GfxDevice(GfxDevice&&) = default;
		GfxDevice& operator=(GfxDevice const&) = delete;
		GfxDevice& operator=(GfxDevice&&) = default;
		~GfxDevice();

		void ResizeBackbuffer(uint32 w, uint32 h);
		void ClearBackbuffer();
		void SwapBuffers(bool vsync);
		void SetBackbuffer();

		ID3D11Device* Device() const;
		ID3D11DeviceContext* Context() const;
		ID3DUserDefinedAnnotation* Annotation() const;

	private:
		uint32 width, height;
		Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain = nullptr;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backbuffer_rtv = nullptr;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> backbuffer = nullptr;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context = nullptr;
		Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> annot = nullptr;
		
	private:

		void WaitForGPU();
		void CreateBackBufferResources(uint32 w, uint32 h);
	};
}