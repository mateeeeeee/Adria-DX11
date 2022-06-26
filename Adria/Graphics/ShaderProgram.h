#pragma once
#include <wrl.h>
#include "ShaderCompiler.h"
#include "../Core/Macros.h" 

namespace adria
{
	class Shader
	{
	public:
		Shader(ShaderBlob const& blob, EShaderStage stage)
			: blob(blob), stage(stage)
		{}

		ShaderBlob& GetBlob() { return blob; }
		EShaderStage GetStage() const { return stage; }

		virtual ~Shader() = default;
		virtual void Bind(ID3D11DeviceContext* context) = 0;
		virtual void Unbind(ID3D11DeviceContext* context) = 0;
		virtual void Recreate(ID3D11Device* device, ShaderBlob const& blob) = 0;
	private:
		ShaderBlob blob;
		EShaderStage stage;
	};
	class VertexShader final : public Shader
	{
		friend struct ShaderProgram;

	public:
		VertexShader(ID3D11Device* device, ShaderBlob const& blob) : Shader(blob, EShaderStage::VS)
		{
			ShaderBlob const& vs_blob = GetBlob();
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
		virtual void Recreate(ID3D11Device* device, ShaderBlob const& blob) override
		{
			GetBlob() = blob;
			HRESULT hr = device->CreateVertexShader(blob.GetPointer(), blob.GetLength(), nullptr, vs.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vs = nullptr;
	};
	class PixelShader final : public Shader
	{
		friend struct ShaderProgram;

	public:
		PixelShader(ID3D11Device* device, ShaderBlob const& blob) : Shader(blob, EShaderStage::PS)
		{
			ShaderBlob const& ps_blob = GetBlob();
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
		virtual void Recreate(ID3D11Device* device, ShaderBlob const& blob) override
		{
			GetBlob() = blob;
			HRESULT hr = device->CreatePixelShader(blob.GetPointer(), blob.GetLength(), nullptr, ps.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11PixelShader> ps = nullptr;
	};
	class GeometryShader final : public Shader
	{
		friend struct ShaderProgram;

	public:
		GeometryShader(ID3D11Device* device, ShaderBlob const& blob) : Shader(blob, EShaderStage::GS)
		{
			ShaderBlob const& gs_blob = GetBlob();
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
		virtual void Recreate(ID3D11Device* device, ShaderBlob const& blob) override
		{
			GetBlob() = blob;
			HRESULT hr = device->CreateGeometryShader(blob.GetPointer(), blob.GetLength(), nullptr, gs.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11GeometryShader> gs = nullptr;
	};
	class DomainShader final : public Shader
	{
		friend struct ShaderProgram;

	public:
		DomainShader(ID3D11Device* device, ShaderBlob const& blob) : Shader(blob, EShaderStage::DS)
		{
			ShaderBlob const& ds_blob = GetBlob();
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
		virtual void Recreate(ID3D11Device* device, ShaderBlob const& blob) override
		{
			GetBlob() = blob;
			HRESULT hr = device->CreateDomainShader(blob.GetPointer(), blob.GetLength(), nullptr, ds.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11DomainShader> ds = nullptr;
	};
	class HullShader final : public Shader
	{
		friend struct ShaderProgram;

	public:
		HullShader(ID3D11Device* device, ShaderBlob const& blob) : Shader(blob, EShaderStage::HS)
		{
			ShaderBlob const& hs_blob = GetBlob();
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
		virtual void Recreate(ID3D11Device* device, ShaderBlob const& blob) override
		{
			GetBlob() = blob;
			HRESULT hr = device->CreateHullShader(blob.GetPointer(), blob.GetLength(), nullptr, hs.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11HullShader> hs = nullptr;
	};
	class ComputeShader final : public Shader
	{
		friend struct ShaderProgram;

	public:
		ComputeShader(ID3D11Device* device, ShaderBlob const& blob) : Shader(blob, EShaderStage::CS)
		{
			ShaderBlob const& cs_blob = GetBlob();
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
		virtual void Recreate(ID3D11Device* device, ShaderBlob const& blob) override
		{
			GetBlob() = blob;
			HRESULT hr = device->CreateComputeShader(blob.GetPointer(), blob.GetLength(), nullptr, cs.ReleaseAndGetAddressOf());
			BREAK_IF_FAILED(hr);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> cs = nullptr;
	};

	class InputLayout
	{
	public:
		InputLayout() {}

		InputLayout(ID3D11Device* device, ShaderBlob const& vs_blob, std::vector<D3D11_INPUT_ELEMENT_DESC> const& desc = {})
		{
			if (!desc.empty()) device->CreateInputLayout(desc.data(), (UINT)desc.size(), vs_blob.GetPointer(), vs_blob.GetLength(),
				layout.GetAddressOf());
			else
				ShaderCompiler::CreateInputLayoutWithReflection(device, vs_blob, layout.GetAddressOf());
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

	struct ShaderProgram
	{
		virtual ~ShaderProgram() = default;
		virtual void Bind(ID3D11DeviceContext* context) = 0;
		virtual void Unbind(ID3D11DeviceContext* context) = 0;
	};

	class GraphicsShaderProgram final : public ShaderProgram
	{
		enum ShaderSlot
		{
			VS = 0,
			PS = 1,
			DS = 2,
			HS = 3,
			GS = 4,
			ShaderCount = 5
		};

	public:
		GraphicsShaderProgram() = default;
		GraphicsShaderProgram& SetVertexShader(VertexShader* vs)
		{
			shaders[VS] = vs;
			return *this;
		}
		GraphicsShaderProgram& SetPixelShader(PixelShader* ps)
		{
			shaders[PS] = ps;
			return *this;
		}
		GraphicsShaderProgram& SetDomainShader(DomainShader* ds)
		{
			shaders[DS] = ds;
			return *this;
		}
		GraphicsShaderProgram& SetHullShader(HullShader* hs)
		{
			shaders[HS] = hs;
			return *this;
		}
		GraphicsShaderProgram& SetGeometryShader(GeometryShader* gs)
		{
			shaders[GS] = gs;
			return *this;
		}
		GraphicsShaderProgram& SetInputLayout(InputLayout* il)
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
		Shader* shaders[ShaderCount] = { nullptr };
		InputLayout* input_layout = nullptr;
	};
	class ComputeShaderProgram final : public ShaderProgram
	{
	public:
		ComputeShaderProgram() = default;

		void SetComputeShader(ComputeShader* cs)
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
		ComputeShader* shader = nullptr;
	};
}