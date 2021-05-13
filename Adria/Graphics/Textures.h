#pragma once
#include <d3d11.h>
#include <wrl.h>
#include "../Utilities/TemplatesUtil.h"

namespace adria
{
	namespace [[deprecated]] deprecated
	{

	template<typename...>
	struct Texture2D;

	template<typename View, typename... Views>
	class Texture2D<View, Views...> : public Texture2D<Views...>
	{

		template<typename View>
		static constexpr UINT GetFlag()
		{
			if constexpr (std::is_same_v<View, ID3D11ShaderResourceView>)
				return D3D11_BIND_SHADER_RESOURCE;
			else if constexpr (std::is_same_v<View, ID3D11RenderTargetView>)
				return D3D11_BIND_RENDER_TARGET;
			else if constexpr (std::is_same_v<View, ID3D11UnorderedAccessView>)
				return D3D11_BIND_UNORDERED_ACCESS;
			else if constexpr (std::is_same_v<View, ID3D11DepthStencilView>)
				return D3D11_BIND_DEPTH_STENCIL;
			else return 0U;
		}

		template<typename View>
		static constexpr bool IsNonDepthView()
		{
			return GetFlag<View>() != 0 && GetFlag<View>() != D3D11_BIND_DEPTH_STENCIL;
		}

		//unique_types<View, Views...> unique{};
		using BaseTexture = Texture2D<Views...>;
		static_assert(IsNonDepthView<View>() , "type View is not a valid view");

	public:

		Texture2D() = default;

		Texture2D(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format, UINT flags = 0)
			: BaseTexture(device, width, height, format, flags | GetFlag<View>())
		{
			CreateView<View>(device);
			(CreateView<Views>(device), ...);
		}

		Texture2D(ID3D11Device* device, D3D11_TEXTURE2D_DESC const& desc)
			: BaseTexture(device, desc)
		{
			CreateView<View>(device);
			(CreateView<Views>(device), ...);
		}

		void OnResize(ID3D11Device* device, UINT width, UINT height)
		{
			ID3D11Texture2D* tex = nullptr;
			view->GetResource(reinterpret_cast<ID3D11Resource**>(&tex));
			D3D11_TEXTURE2D_DESC desc;
			tex->GetDesc(&desc);

			DXGI_FORMAT format = desc.Format;

			*this = Texture2D(device, width, height, format);

			
		}

		
		auto SRV() const
		{
			return Get<ID3D11ShaderResourceView>();
		}

		auto UAV() const
		{
			return Get<ID3D11UnorderedAccessView>();
		}

		auto RTV() const
		{
			return Get<ID3D11RenderTargetView>();
		}

		auto RawSRV() const
		{
			return GetRaw<ID3D11ShaderResourceView>();
		}

		auto RawUAV() const
		{
			return GetRaw<ID3D11UnorderedAccessView>();
		}

		auto RawRTV() const
		{
			return GetRaw<ID3D11RenderTargetView>();
		}

		
	protected:

		template<typename V>
		void CreateView(ID3D11Device* device) //add argument view_desc_t with mip levels, detailed mip, mip slice etc?
		{
			if constexpr (std::is_same_v<View, V>)
			{
				D3D11_TEXTURE2D_DESC desc{};

				auto tex = BaseTexture::tex.Get();

				tex->GetDesc(&desc);

				if constexpr (std::is_same_v<View, ID3D11ShaderResourceView>)
				{
					D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc =
					{
						desc.Format,
						D3D11_SRV_DIMENSION_TEXTURE2D,
						0, //most detailed mip
						1  //mip levels
					};

					device->CreateShaderResourceView(tex, &srv_desc, view.GetAddressOf());
				}
				else if constexpr (std::is_same_v<View, ID3D11RenderTargetView>)
				{
					D3D11_RENDER_TARGET_VIEW_DESC rtv_desc =
					{
						desc.Format,
						D3D11_RTV_DIMENSION_TEXTURE2D
					};

					device->CreateRenderTargetView(tex, &rtv_desc, view.GetAddressOf());

				}

				else if constexpr (std::is_same_v<View, ID3D11UnorderedAccessView>)
				{
					D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
					ZeroMemory(&uav_desc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
					uav_desc.Format = desc.Format;
					uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
					uav_desc.Texture2D.MipSlice = 0;
					device->CreateUnorderedAccessView(tex, &uav_desc, view.GetAddressOf());
				}
			}
			else  BaseTexture::template CreateView<V>(device);
		}

		template<typename V>
		auto Get() const
		{
			if constexpr (std::is_same<View, V>::value)
			{
				return view;
			}
			else return BaseTexture::template Get<V>();
		}

		template<typename V>
		V* GetRaw() const
		{
			auto view = Get<V>();
			return view.Get();
		}

	private:
		
		Microsoft::WRL::ComPtr<View> view;

	};


	template<>
	class Texture2D<>
	{
	protected:

		Texture2D() = default;

		Texture2D(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format,  UINT flags)
		{
			D3D11_TEXTURE2D_DESC tex_desc =
			{
					width, 
					height, 
					1, //UINT MipLevels;
					1, //UINT ArraySize;
					format,
					1,
					0,
					D3D11_USAGE_DEFAULT,
					flags, 
					0,
					0
			};

			device->CreateTexture2D(&tex_desc, nullptr, tex.GetAddressOf());
		}

		Texture2D(ID3D11Device* device, D3D11_TEXTURE2D_DESC const& tex_desc)
		{
			device->CreateTexture2D(&tex_desc, nullptr, tex.GetAddressOf());
		}

		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex = nullptr;

	};

	//specializations for depth texture
	template<> 
	class Texture2D<ID3D11DepthStencilView, ID3D11ShaderResourceView>
	{
	public:
		Texture2D() = default;

		Texture2D(ID3D11Device* device, UINT width, UINT height, bool use_stencil)
		{
			CreateTextureViews(device, width, height, use_stencil);
		}

		void OnResize(ID3D11Device* device, UINT width, UINT height)
		{
			ID3D11Texture2D* tex = nullptr;
			dsv->GetResource(reinterpret_cast<ID3D11Resource**>(&tex));
			D3D11_TEXTURE2D_DESC desc;
			tex->GetDesc(&desc);

			bool use_stencil = desc.Format == DXGI_FORMAT_R24G8_TYPELESS;

			CreateTextureViews(device, width, height, use_stencil);
		}

		auto SRV() const
		{
			return srv;
		}

		auto DSV() const
		{
			return dsv;
		}

		ID3D11ShaderResourceView* RawSRV() const
		{
			return srv.Get();
		}

		ID3D11DepthStencilView* RawDSV() const
		{
			return dsv.Get();
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

	private:

		void CreateTextureViews(ID3D11Device* device, UINT width, UINT height, bool use_stencil)
		{
			D3D11_TEXTURE2D_DESC texDesc{};
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.MipLevels = 1;
			texDesc.ArraySize = 1;
			texDesc.Format = use_stencil ? DXGI_FORMAT_R24G8_TYPELESS : DXGI_FORMAT_R32_TYPELESS;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;


			Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_buffer_tex = nullptr;
			device->CreateTexture2D(&texDesc, nullptr, depth_buffer_tex.GetAddressOf());



			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
			dsvDesc.Flags = 0;
			dsvDesc.Format = use_stencil ? DXGI_FORMAT_D24_UNORM_S8_UINT : DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;

			device->CreateDepthStencilView(depth_buffer_tex.Get(), &dsvDesc, dsv.GetAddressOf());


			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = use_stencil ? DXGI_FORMAT_R24_UNORM_X8_TYPELESS : DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
			srvDesc.Texture2D.MostDetailedMip = 0;

			device->CreateShaderResourceView(depth_buffer_tex.Get(), &srvDesc, srv.GetAddressOf());
		}
	};

	template<> 
	class Texture2D<ID3D11DepthStencilView>
	{
	public:
		Texture2D() = default;

		Texture2D(ID3D11Device* device, UINT width, UINT height, bool use_stencil)
		{
			CreateTextureViews(device, width, height, use_stencil);
		}

		void OnResize(ID3D11Device* device, UINT width, UINT height)
		{
			ID3D11Texture2D* tex = nullptr;
			dsv->GetResource(reinterpret_cast<ID3D11Resource**>(&tex));
			D3D11_TEXTURE2D_DESC desc;
			tex->GetDesc(&desc);

			bool use_stencil = desc.Format == DXGI_FORMAT_R24G8_TYPELESS;

			CreateTextureViews(device, width, height, use_stencil);
		}

		auto DSV() const
		{
			return dsv;
		}

		ID3D11DepthStencilView* RawDSV() const
		{
			return dsv.Get();
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> dsv;
		
	private:

		void CreateTextureViews(ID3D11Device* device, UINT width, UINT height, bool use_stencil)
		{
			D3D11_TEXTURE2D_DESC texDesc{};
			texDesc.Width = width;
			texDesc.Height = height;
			texDesc.MipLevels = 1;
			texDesc.ArraySize = 1;
			texDesc.Format = use_stencil ? DXGI_FORMAT_R24G8_TYPELESS : DXGI_FORMAT_R32_TYPELESS;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;


			Microsoft::WRL::ComPtr<ID3D11Texture2D> depth_buffer_tex = nullptr;
			device->CreateTexture2D(&texDesc, nullptr, depth_buffer_tex.GetAddressOf());

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
			dsvDesc.Flags = 0;
			dsvDesc.Format = use_stencil ? DXGI_FORMAT_D24_UNORM_S8_UINT : DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;

			device->CreateDepthStencilView(depth_buffer_tex.Get(), &dsvDesc, dsv.GetAddressOf());

		}
	};

	using RtvSrvTexture		= Texture2D<ID3D11RenderTargetView,ID3D11ShaderResourceView>;
	using UavSrvTexture		= Texture2D<ID3D11UnorderedAccessView, ID3D11ShaderResourceView>;
	using DsvSrvTexture		= Texture2D<ID3D11DepthStencilView, ID3D11ShaderResourceView>;
	using RtvUavSrvTexture	= Texture2D<ID3D11RenderTargetView, ID3D11UnorderedAccessView, ID3D11ShaderResourceView>;
	
	}
}