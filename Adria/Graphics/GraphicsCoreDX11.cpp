#include "GraphicsCoreDX11.h"
#include "../Core/Macros.h"



namespace adria
{
	GraphicsCoreDX11::GraphicsCoreDX11(void* handle)
	{
		HWND w_handle = static_cast<HWND>(handle);
		RECT rect;
		GetClientRect(w_handle, &rect);
		width	= rect.right - rect.left;
		height	= rect.bottom - rect.top;

		u32 swapCreateFlags = 0;

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
			&swapchain,
			&device,
			nullptr,
			&immediate_context
		);

		BREAK_IF_FAILED(hr);


#if defined(_DEBUG) || defined(FORCE_DEBUG_LAYER)
		Microsoft::WRL::ComPtr<ID3D11Debug> d3dDebug;
		hr = device.As(&d3dDebug);
		if (SUCCEEDED(hr))
		{
			Microsoft::WRL::ComPtr<ID3D11InfoQueue> d3dInfoQueue;
			hr = d3dDebug.As(&d3dInfoQueue);
			if (SUCCEEDED(hr))
			{
				D3D11_MESSAGE_ID hide[] =
				{
					D3D11_MESSAGE_ID_DEVICE_DRAW_RESOURCE_FORMAT_SAMPLE_C_UNSUPPORTED
					// TODO: Add more message IDs here as needed 
				};
				D3D11_INFO_QUEUE_FILTER filter{};
				filter.DenyList.NumIDs = _countof(hide);
				filter.DenyList.pIDList = hide;
				d3dInfoQueue->AddStorageFilterEntries(&filter);
			}
		}
#endif

		CreateBackBufferResources(width, height);

	}
	GraphicsCoreDX11::~GraphicsCoreDX11()
	{
		WaitForGPU();
	}
	void GraphicsCoreDX11::ResizeBackbuffer(u32 w, u32 h)
	{
		if ((width != w || height != h) && w > 0 && h > 0)
		{
			width = w;
			height = h;
			CreateBackBufferResources(w, h);
		}
	}
	void GraphicsCoreDX11::ClearBackbuffer()
	{
		f32 clear_color[] = { 0.0f,0.0f, 0.0f,0.0f };
		immediate_context->ClearRenderTargetView(backbuffer_rtv.Get(), clear_color);
	}
	void GraphicsCoreDX11::SwapBuffers(bool vsync)
	{
		HRESULT hr = swapchain->Present(vsync, 0);
		BREAK_IF_FAILED(hr);
	}
	void GraphicsCoreDX11::SetBackbuffer()
	{
		D3D11_VIEWPORT vp = {};
		vp.Width = static_cast<f32>(width);
		vp.Height = static_cast<f32>(height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		immediate_context->RSSetViewports(1, &vp);
		immediate_context->OMSetRenderTargets(1, backbuffer_rtv.GetAddressOf(), nullptr);
	}
	ID3D11Device* GraphicsCoreDX11::Device() const
	{
		return device.Get();
	}
	ID3D11DeviceContext* GraphicsCoreDX11::Context() const
	{
		return immediate_context.Get();
	}
	void GraphicsCoreDX11::WaitForGPU()
	{
		immediate_context->Flush();
		D3D11_QUERY_DESC desc{};
		desc.MiscFlags = 0;
		desc.Query = D3D11_QUERY_EVENT;
		Microsoft::WRL::ComPtr<ID3D11Query> query = nullptr;
		HRESULT hr = device->CreateQuery(&desc, &query);
		ADRIA_ASSERT(SUCCEEDED(hr));
		immediate_context->End(query.Get());
		BOOL result;
		while (immediate_context->GetData(query.Get(), &result, sizeof(result), 0) == S_FALSE);
		ADRIA_ASSERT(result == TRUE);
	}
	void GraphicsCoreDX11::CreateBackBufferResources(u32 w, u32 h)
	{
		immediate_context->OMSetRenderTargets(0, 0, 0);

		if (backbuffer_rtv) backbuffer_rtv->Release();

		BREAK_IF_FAILED(swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));

		ID3D11Texture2D* pBuffer = nullptr;
		BREAK_IF_FAILED(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer));
		BREAK_IF_FAILED(device->CreateRenderTargetView(pBuffer, nullptr,
			backbuffer_rtv.GetAddressOf()));

		pBuffer->Release();

		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = w;
		depthStencilDesc.Height = h;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;

		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		immediate_context->OMSetRenderTargets(1, backbuffer_rtv.GetAddressOf(), nullptr);
		D3D11_VIEWPORT vp{};
		vp.Width = static_cast<f32>(w);
		vp.Height = static_cast<f32>(h);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		immediate_context->RSSetViewports(1, &vp);
	}
}