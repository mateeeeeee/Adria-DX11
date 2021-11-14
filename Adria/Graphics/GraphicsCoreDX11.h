#pragma once
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "dxguid.lib")

#include <d3d11_1.h>
#include <wrl.h>
#include <dxgi1_3.h>
#include "../Core/Definitions.h"

namespace adria
{
	class GraphicsCoreDX11
	{
	public:
		GraphicsCoreDX11(void* handle);
		GraphicsCoreDX11(GraphicsCoreDX11 const&) = delete;
		GraphicsCoreDX11(GraphicsCoreDX11&&) = default;
		GraphicsCoreDX11& operator=(GraphicsCoreDX11 const&) = delete;
		GraphicsCoreDX11& operator=(GraphicsCoreDX11&&) = default;
		~GraphicsCoreDX11();

		void ResizeBackbuffer(u32 w, u32 h);
		void ClearBackbuffer();
		void SwapBuffers(bool vsync);
		void SetBackbuffer();

		ID3D11Device* Device() const;
		ID3D11DeviceContext* Context() const;
		ID3DUserDefinedAnnotation* Annotation() const;

	private:
		u32 width, height;
		Microsoft::WRL::ComPtr<ID3D11Device> device = nullptr;
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain = nullptr;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backbuffer_rtv = nullptr;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> backbuffer = nullptr;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context = nullptr;
		ID3DUserDefinedAnnotation* annot = nullptr;
		
	private:

		void WaitForGPU();

		void CreateBackBufferResources(u32 w, u32 h);
	};
}