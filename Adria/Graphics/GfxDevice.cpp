#include <dxgidebug.h>
#include "GfxDevice.h"
#include "GfxCommandContext.h"
#include "Core/Window.h"

#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "dxguid.lib")

namespace adria
{
	static inline void ReportLiveObjects()
	{
		Ref<IDXGIDebug1> dxgi_debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgi_debug.GetAddressOf()))))
		{
			dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		}
	}

	GfxDevice::GfxDevice(Window* window) : window(window), command_context(new GfxCommandContext(this))
	{
		width	= window->Width();
		height	= window->Height();

		std::atexit(ReportLiveObjects);
		Uint32 swapchain_create_flags = 0;
#if defined(_DEBUG) 
		swapchain_create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		DXGI_SWAP_CHAIN_DESC swapchain_desc{};
		swapchain_desc.BufferDesc.Width = 0;
		swapchain_desc.BufferDesc.Height = 0;
		swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		swapchain_desc.BufferDesc.RefreshRate.Numerator = 0;
		swapchain_desc.BufferDesc.RefreshRate.Denominator = 0;
		swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapchain_desc.SampleDesc.Count = 1;
		swapchain_desc.SampleDesc.Quality = 0;
		swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchain_desc.BufferCount = GFX_BACKBUFFER_COUNT;
		swapchain_desc.OutputWindow = (HWND)window->Handle();
		swapchain_desc.Windowed = TRUE;
		swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapchain_desc.Flags = 0;

		ID3D11DeviceContext* context = nullptr;
		ID3D11Device* _device = nullptr;
		D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_12_0;
		HRESULT hr = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			swapchain_create_flags,
			&feature_level,
			1,
			D3D11_SDK_VERSION,
			&swapchain_desc,
			swapchain.GetAddressOf(),
			&_device,
			nullptr,
			&context
		);
		hr = _device->QueryInterface(__uuidof(ID3D11Device3), (void**)device.GetAddressOf());
		_device->Release();

		GFX_CHECK_HR(hr);
		command_context->Create(context);
		
#if defined(_DEBUG)
		Ref<ID3D11Debug> d3d_debug;
		hr = device.As(&d3d_debug);
		if (SUCCEEDED(hr))
		{
			Ref<ID3D11InfoQueue> d3d_info_queue;
			hr = d3d_debug.As(&d3d_info_queue);
			if (SUCCEEDED(hr))
			{
				D3D11_MESSAGE_ID hide[] =
				{
					D3D11_MESSAGE_ID_DEVICE_DRAW_RESOURCE_FORMAT_SAMPLE_C_UNSUPPORTED,
					D3D11_MESSAGE_ID_QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS,
					D3D11_MESSAGE_ID_QUERY_END_ABANDONING_PREVIOUS_RESULTS
				};
				D3D11_INFO_QUEUE_FILTER filter{};
				filter.DenyList.NumIDs = ARRAYSIZE(hide);
				filter.DenyList.pIDList = hide;
				d3d_info_queue->AddStorageFilterEntries(&filter);
				d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
				d3d_info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
			}
		}
#endif
		CreateBackBufferResources(width, height);
	}
	GfxDevice::~GfxDevice()
	{
		WaitForGPU();
	}
	void GfxDevice::ResizeBackbuffer(Uint32 w, Uint32 h)
	{
		if ((width != w || height != h) && w > 0 && h > 0)
		{
			width = w;
			height = h;
			CreateBackBufferResources(w, h);
		}
	}
	void GfxDevice::ClearBackbuffer()
	{
		command_context->Begin();
		Float clear_color[] = { 0.0f,0.0f, 0.0f,0.0f };
		command_context->ClearRenderTarget(backbuffer_rtv.Get(), clear_color);
	}
	void GfxDevice::SwapBuffers(Bool vsync)
	{
		GFX_CHECK_HR(swapchain->Present(vsync, 0));
		command_context->End();
	}
	void GfxDevice::SetBackbuffer()
	{
		command_context->SetViewport(0, 0, width, height);
		GfxRenderTarget rtv[] = { backbuffer_rtv.Get() };
		command_context->SetRenderTargets(rtv);
	}
	ID3D11Device3* GfxDevice::GetDevice() const
	{
		return device.Get();
	}
	ID3D11DeviceContext4* GfxDevice::GetContext() const
	{
		return command_context->GetNative();
	}

	void GfxDevice::WaitForGPU()
	{
		command_context->WaitForGPU();
	}
	void GfxDevice::CreateBackBufferResources(Uint32 w, Uint32 h)
	{
		GfxRenderTarget null_views[] = { nullptr };
		command_context->SetRenderTargets(null_views);
		command_context->Flush();

		if (backbuffer_rtv) backbuffer_rtv->Release();
		GFX_CHECK_HR(swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));

		Ref<ID3D11Texture2D> p_buffer = nullptr;
		GFX_CHECK_HR(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)p_buffer.GetAddressOf()));
		GFX_CHECK_HR(device->CreateRenderTargetView(p_buffer.Get(), nullptr, backbuffer_rtv.GetAddressOf()));

		GfxRenderTarget rtv[] = { backbuffer_rtv.Get()};
		command_context->SetRenderTargets(rtv);
		command_context->SetViewport(0, 0, w, h);
	}
}