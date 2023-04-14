#pragma once
#include <wrl.h>
#include "GfxShaderCompiler.h"
#include "Core/Defines.h" 

namespace adria
{
	class GfxShader
	{
	public:
		GfxShader(GfxShaderBlob const& blob, GfxShaderStage stage)
			: blob(blob), stage(stage)
		{}

		GfxShaderBlob& GetBlob() { return blob; }
		GfxShaderStage GetStage() const { return stage; }

		virtual ~GfxShader() = default;
		virtual void Bind(ID3D11DeviceContext* context) = 0;
		virtual void Unbind(ID3D11DeviceContext* context) = 0;
		virtual void Recreate(ID3D11Device* device, GfxShaderBlob const& blob) = 0;
	private:
		GfxShaderBlob blob;
		GfxShaderStage stage;
	};
	class GfxVertexShader final : public GfxShader
	{
		friend struct GfxShaderProgram;

	public:
		GfxVertexShader(ID3D11Device* device, GfxShaderBlob const& blob) : GfxShader(blob, GfxShaderStage::VS)
		{
			GfxShaderBlob const& vs_blob = GetBlob();
			HRESULT hr = device->CreateVertexShader(vs_blob.GetPointer(), vs_blob.GetLength(), nullptr, vs.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}
		virtual void Bind(ID3D11DeviceContext* context) override
		{
			context->VSSetShader(vs.Get(), nullptr, 0);
		}
		virtual void Unbind(ID3D11DeviceContext* context) override
		{
			context->VSSetShader(nullptr, nullptr, 0);
		}
		virtual void Recreate(ID3D11Device* device, GfxShaderBlob const& blob) override
		{
			GetBlob() = blob;
			HRESULT hr = device->CreateVertexShader(blob.GetPointer(), blob.GetLength(), nullptr, vs.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vs = nullptr;
	};
	class GfxPixelShader final : public GfxShader
	{
		friend struct GfxShaderProgram;

	public:
		GfxPixelShader(ID3D11Device* device, GfxShaderBlob const& blob) : GfxShader(blob, GfxShaderStage::PS)
		{
			GfxShaderBlob const& ps_blob = GetBlob();
			HRESULT hr = device->CreatePixelShader(ps_blob.GetPointer(), ps_blob.GetLength(), nullptr, ps.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}

		virtual void Bind(ID3D11DeviceContext* context) override
		{
			context->PSSetShader(ps.Get(), nullptr, 0);
		}
		virtual void Unbind(ID3D11DeviceContext* context) override
		{
			context->PSSetShader(nullptr, nullptr, 0);
		}
		virtual void Recreate(ID3D11Device* device, GfxShaderBlob const& blob) override
		{
			GetBlob() = blob;
			HRESULT hr = device->CreatePixelShader(blob.GetPointer(), blob.GetLength(), nullptr, ps.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11PixelShader> ps = nullptr;
	};
	class GfxGeometryShader final : public GfxShader
	{
		friend struct GfxShaderProgram;

	public:
		GfxGeometryShader(ID3D11Device* device, GfxShaderBlob const& blob) : GfxShader(blob, GfxShaderStage::GS)
		{
			GfxShaderBlob const& gs_blob = GetBlob();
			HRESULT hr = device->CreateGeometryShader(gs_blob.GetPointer(), gs_blob.GetLength(), nullptr, gs.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}

		virtual void Bind(ID3D11DeviceContext* context) override
		{
			context->GSSetShader(gs.Get(), nullptr, 0);
		}
		virtual void Unbind(ID3D11DeviceContext* context) override
		{
			context->GSSetShader(nullptr, nullptr, 0);
		}
		virtual void Recreate(ID3D11Device* device, GfxShaderBlob const& blob) override
		{
			GetBlob() = blob;
			HRESULT hr = device->CreateGeometryShader(blob.GetPointer(), blob.GetLength(), nullptr, gs.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11GeometryShader> gs = nullptr;
	};
	class GfxDomainShader final : public GfxShader
	{
		friend struct GfxShaderProgram;

	public:
		GfxDomainShader(ID3D11Device* device, GfxShaderBlob const& blob) : GfxShader(blob, GfxShaderStage::DS)
		{
			GfxShaderBlob const& ds_blob = GetBlob();
			HRESULT hr = device->CreateDomainShader(ds_blob.GetPointer(), ds_blob.GetLength(), nullptr, ds.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}
		virtual void Bind(ID3D11DeviceContext* context) override
		{
			context->DSSetShader(ds.Get(), nullptr, 0);
		}
		virtual void Unbind(ID3D11DeviceContext* context) override
		{
			context->DSSetShader(nullptr, nullptr, 0);
		}
		virtual void Recreate(ID3D11Device* device, GfxShaderBlob const& blob) override
		{
			GetBlob() = blob;
			HRESULT hr = device->CreateDomainShader(blob.GetPointer(), blob.GetLength(), nullptr, ds.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11DomainShader> ds = nullptr;
	};
	class GfxHullShader final : public GfxShader
	{
		friend struct GfxShaderProgram;

	public:
		GfxHullShader(ID3D11Device* device, GfxShaderBlob const& blob) : GfxShader(blob, GfxShaderStage::HS)
		{
			GfxShaderBlob const& hs_blob = GetBlob();
			HRESULT hr = device->CreateHullShader(hs_blob.GetPointer(), hs_blob.GetLength(), nullptr, hs.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}
		virtual void Bind(ID3D11DeviceContext* context) override
		{
			context->HSSetShader(hs.Get(), nullptr, 0);
		}
		virtual void Unbind(ID3D11DeviceContext* context) override
		{
			context->HSSetShader(nullptr, nullptr, 0);
		}
		virtual void Recreate(ID3D11Device* device, GfxShaderBlob const& blob) override
		{
			GetBlob() = blob;
			HRESULT hr = device->CreateHullShader(blob.GetPointer(), blob.GetLength(), nullptr, hs.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11HullShader> hs = nullptr;
	};
	class GfxComputeShader final : public GfxShader
	{
		friend struct GfxShaderProgram;

	public:
		GfxComputeShader(ID3D11Device* device, GfxShaderBlob const& blob) : GfxShader(blob, GfxShaderStage::CS)
		{
			GfxShaderBlob const& cs_blob = GetBlob();
			HRESULT hr = device->CreateComputeShader(cs_blob.GetPointer(), cs_blob.GetLength(), nullptr, cs.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}
		virtual void Bind(ID3D11DeviceContext* context) override
		{
			context->CSSetShader(cs.Get(), nullptr, 0);
		}
		virtual void Unbind(ID3D11DeviceContext* context) override
		{
			context->CSSetShader(nullptr, nullptr, 0);
		}
		virtual void Recreate(ID3D11Device* device, GfxShaderBlob const& blob) override
		{
			GetBlob() = blob;
			HRESULT hr = device->CreateComputeShader(blob.GetPointer(), blob.GetLength(), nullptr, cs.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> cs = nullptr;
	};

	class GfxInputLayout
	{
	public:
		GfxInputLayout() {}

		GfxInputLayout(ID3D11Device* device, GfxShaderBlob const& vs_blob, std::vector<D3D11_INPUT_ELEMENT_DESC> const& desc = {})
		{
			if (!desc.empty()) device->CreateInputLayout(desc.data(), (UINT)desc.size(), vs_blob.GetPointer(), vs_blob.GetLength(),
				layout.GetAddressOf());
			else
				GfxShaderCompiler::CreateInputLayoutWithReflection(device, vs_blob, layout.GetAddressOf());
		}

		void Bind(ID3D11DeviceContext* context)
		{
			context->IASetInputLayout(layout.Get());
		}
		void Unbind(ID3D11DeviceContext* context)
		{
			context->IASetInputLayout(nullptr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11InputLayout> layout;
	};

	struct GfxShaderProgram
	{
		virtual ~GfxShaderProgram() = default;
		virtual void Bind(ID3D11DeviceContext* context) = 0;
		virtual void Unbind(ID3D11DeviceContext* context) = 0;
	};

	class GfxGraphicsShaderProgram final : public GfxShaderProgram
	{
		enum GfxShaderSlot
		{
			VS = 0,
			PS = 1,
			DS = 2,
			HS = 3,
			GS = 4,
			ShaderCount = 5
		};

	public:
		GfxGraphicsShaderProgram() = default;
		GfxGraphicsShaderProgram& SetVertexShader(GfxVertexShader* vs)
		{
			shaders[VS] = vs;
			return *this;
		}
		GfxGraphicsShaderProgram& SetPixelShader(GfxPixelShader* ps)
		{
			shaders[PS] = ps;
			return *this;
		}
		GfxGraphicsShaderProgram& SetDomainShader(GfxDomainShader* ds)
		{
			shaders[DS] = ds;
			return *this;
		}
		GfxGraphicsShaderProgram& SetHullShader(GfxHullShader* hs)
		{
			shaders[HS] = hs;
			return *this;
		}
		GfxGraphicsShaderProgram& SetGeometryShader(GfxGeometryShader* gs)
		{
			shaders[GS] = gs;
			return *this;
		}
		GfxGraphicsShaderProgram& SetInputLayout(GfxInputLayout* il)
		{
			input_layout = il;
			return *this;
		}

		virtual void Bind(ID3D11DeviceContext* context) override
		{
			if (input_layout) input_layout->Bind(context);
			for (size_t i = 0; i < ShaderCount; ++i)
				if (shaders[i]) shaders[i]->Bind(context);
		}
		virtual void Unbind(ID3D11DeviceContext* context) override
		{
			if (input_layout) input_layout->Unbind(context);
			for (size_t i = 0; i < ShaderCount; ++i)
				if (shaders[i]) shaders[i]->Unbind(context);
		}

	private:
		GfxShader* shaders[ShaderCount] = { nullptr };
		GfxInputLayout* input_layout = nullptr;
	};
	class GfxComputeShaderProgram final : public GfxShaderProgram
	{
	public:
		GfxComputeShaderProgram() = default;

		void SetComputeShader(GfxComputeShader* cs)
		{
			shader = cs;
		}
		virtual void Bind(ID3D11DeviceContext* context) override
		{
			if (shader) shader->Bind(context);
		}
		virtual void Unbind(ID3D11DeviceContext* context) override
		{
			if (shader) shader->Unbind(context);
		}
	private:
		GfxComputeShader* shader = nullptr;
	};
}