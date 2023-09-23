#include <dxgidebug.h>
#include "GfxDevice.h"
#include "Core/Defines.h"

static inline void ReportLiveObjects()
{
	adria::ArcPtr<IDXGIDebug1> dxgi_debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgi_debug.GetAddressOf()))))
	{
		dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
	}
}

namespace adria
{
	GfxDevice::GfxDevice(void* handle)
	{
		HWND w_handle = static_cast<HWND>(handle);
		RECT rect;
		GetClientRect(w_handle, &rect);
		width	= rect.right - rect.left;
		height	= rect.bottom - rect.top;

		uint32 swapCreateFlags = 0;

#if defined(_DEBUG)  || defined(FORCE_DEBUG_LAYER)
		swapCreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferDesc.Width = 0;
		sd.BufferDesc.Height = 0;
		sd.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 0;
		sd.BufferDesc.RefreshRate.Denominator = 0;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 2;
		sd.OutputWindow = w_handle;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = 0;

		HRESULT hr = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			swapCreateFlags,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&sd,
			swapchain.GetAddressOf(),
			device.GetAddressOf(),
			nullptr,
			immediate_context.GetAddressOf()
		);
		GFX_CHECK_HR(hr);
		immediate_context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)annot.GetAddressOf());

#if defined(_DEBUG) || defined(FORCE_DEBUG_LAYER)
		ArcPtr<ID3D11Debug> d3dDebug;
		hr = device.As(&d3dDebug);
		if (SUCCEEDED(hr))
		{
			ArcPtr<ID3D11InfoQueue> d3dInfoQueue;
			hr = d3dDebug.As(&d3dInfoQueue);
			if (SUCCEEDED(hr))
			{
				D3D11_MESSAGE_ID hide[] =
				{
					D3D11_MESSAGE_ID_DEVICE_DRAW_RESOURCE_FORMAT_SAMPLE_C_UNSUPPORTED
					// TODO: Add more message IDs here as needed 
				};
				D3D11_INFO_QUEUE_FILTER filter{};
				filter.DenyList.NumIDs = ARRAYSIZE(hide);
				filter.DenyList.pIDList = hide;
				d3dInfoQueue->AddStorageFilterEntries(&filter);
				d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
				d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
			}
		}
#endif
		CreateBackBufferResources(width, height);
		std::atexit(ReportLiveObjects);
	}
	GfxDevice::~GfxDevice()
	{
		WaitForGPU();
	}
	void GfxDevice::ResizeBackbuffer(uint32 w, uint32 h)
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
		float clear_color[] = { 0.0f,0.0f, 0.0f,0.0f };
		immediate_context->ClearRenderTargetView(backbuffer_rtv.Get(), clear_color);
	}
	void GfxDevice::SwapBuffers(bool vsync)
	{
		HRESULT hr = swapchain->Present(vsync, 0);
		GFX_CHECK_HR(hr);
	}
	void GfxDevice::SetBackbuffer()
	{
		D3D11_VIEWPORT vp = {};
		vp.Width = static_cast<float>(width);
		vp.Height = static_cast<float>(height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		immediate_context->RSSetViewports(1, &vp);
		immediate_context->OMSetRenderTargets(1, backbuffer_rtv.GetAddressOf(), nullptr);
	}
	ID3D11Device* GfxDevice::Device() const
	{
		return device.Get();
	}
	ID3D11DeviceContext* GfxDevice::Context() const
	{
		return immediate_context.Get();
	}

	ID3DUserDefinedAnnotation* GfxDevice::Annotation() const
	{
		return annot.Get();
	}

	void GfxDevice::WaitForGPU()
	{
		immediate_context->Flush();
		D3D11_QUERY_DESC desc{};
		desc.MiscFlags = 0;
		desc.Query = D3D11_QUERY_EVENT;
		ArcPtr<ID3D11Query> query = nullptr;
		HRESULT hr = device->CreateQuery(&desc, query.GetAddressOf());
		ADRIA_ASSERT(SUCCEEDED(hr));
		immediate_context->End(query.Get());
		BOOL result;
		while (immediate_context->GetData(query.Get(), &result, sizeof(result), 0) == S_FALSE);
		ADRIA_ASSERT(result == TRUE);
	}
	void GfxDevice::CreateBackBufferResources(uint32 w, uint32 h)
	{
		ID3D11RenderTargetView* null_views[] = { nullptr };
		immediate_context->OMSetRenderTargets(ARRAYSIZE(null_views), null_views, nullptr);
		immediate_context->Flush();
		if (backbuffer_rtv) backbuffer_rtv->Release();

		GFX_CHECK_HR(swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));

		ArcPtr<ID3D11Texture2D> p_buffer = nullptr;
		GFX_CHECK_HR(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)p_buffer.GetAddressOf()));
		GFX_CHECK_HR(device->CreateRenderTargetView(p_buffer.Get(), nullptr,
			backbuffer_rtv.GetAddressOf()));

		immediate_context->OMSetRenderTargets(1, backbuffer_rtv.GetAddressOf(), nullptr);
		D3D11_VIEWPORT vp{};
		vp.Width = static_cast<float>(w);
		vp.Height = static_cast<float>(h);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		immediate_context->RSSetViewports(1, &vp);
	}
}