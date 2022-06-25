#pragma once
#include <wrl.h>
#include "ShaderCompiler.h"
#include "../Core/Macros.h" 

namespace adria
{
	class Shader
	{
	protected:
		Shader(ShaderBlob const& blob, EShaderStage stage)
			: blob(blob), stage(stage)
		{}

		ShaderBlob const& GetBlob() const { return blob; }
		EShaderStage GetStage() const { return stage; }

	private:
		ShaderBlob blob;
		EShaderStage stage;
	};
	class VertexShader : public Shader
	{
		friend struct ShaderProgram;

	public:
		VertexShader(ID3D11Device* device, ShaderBlob const& blob) : Shader(blob, EShaderStage::VS)
		{
			HRESULT hr;
			ShaderBlob const& vs_blob = GetBlob();
			hr = device->CreateVertexShader(vs_blob.GetPointer(), vs_blob.GetLength(), nullptr, vs.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}

		void Bind(ID3D11DeviceContext* context)
		{
			context->VSSetShader(vs.Get(), nullptr, 0);
		}

		void Unbind(ID3D11DeviceContext* context)
		{
			context->VSSetShader(nullptr, nullptr, 0);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vs = nullptr;
	};
	class PixelShader : public Shader
	{
		friend struct ShaderProgram;

	public:
		PixelShader(ID3D11Device* device, ShaderBlob const& blob) : Shader(blob, EShaderStage::PS)
		{
			HRESULT hr;
			ShaderBlob const& ps_blob = GetBlob();
			hr = device->CreatePixelShader(ps_blob.GetPointer(), ps_blob.GetLength(), nullptr, ps.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}

		void Bind(ID3D11DeviceContext* context)
		{
			context->PSSetShader(ps.Get(), nullptr, 0);
		}

		void Unbind(ID3D11DeviceContext* context)
		{
			context->PSSetShader(nullptr, nullptr, 0);
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11PixelShader> ps = nullptr;
	};
	class GeometryShader : public Shader
	{
		friend struct ShaderProgram;

	public:
		GeometryShader(ID3D11Device* device, ShaderBlob const& blob) : Shader(blob, EShaderStage::GS)
		{
			HRESULT hr;
			ShaderBlob const& gs_blob = GetBlob();
			hr = device->CreateGeometryShader(gs_blob.GetPointer(), gs_blob.GetLength(), nullptr, gs.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}

		void Bind(ID3D11DeviceContext* context)
		{
			context->GSSetShader(gs.Get(), nullptr, 0);
		}

		void Unbind(ID3D11DeviceContext* context)
		{
			context->GSSetShader(nullptr, nullptr, 0);
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11GeometryShader> gs = nullptr;
	};
	class DomainShader : public Shader
	{
		friend struct ShaderProgram;

	public:
		DomainShader(ID3D11Device* device, ShaderBlob const& blob) : Shader(blob, EShaderStage::DS)
		{
			HRESULT hr;
			ShaderBlob const& ds_blob = GetBlob();
			hr = device->CreateDomainShader(ds_blob.GetPointer(), ds_blob.GetLength(), nullptr, ds.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}
		void Bind(ID3D11DeviceContext* context)
		{
			context->DSSetShader(ds.Get(), nullptr, 0);
		}
		void Unbind(ID3D11DeviceContext* context)
		{
			context->DSSetShader(nullptr, nullptr, 0);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11DomainShader> ds = nullptr;
	};
	class HullShader : public Shader
	{
		friend struct ShaderProgram;

	public:
		HullShader(ID3D11Device* device, ShaderBlob const& blob) : Shader(blob, EShaderStage::HS)
		{
			HRESULT hr;
			ShaderBlob const& hs_blob = GetBlob();
			hr = device->CreateHullShader(hs_blob.GetPointer(), hs_blob.GetLength(), nullptr, hs.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}
		void Bind(ID3D11DeviceContext* context)
		{
			context->HSSetShader(hs.Get(), nullptr, 0);
		}
		void Unbind(ID3D11DeviceContext* context)
		{
			context->HSSetShader(nullptr, nullptr, 0);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11HullShader> hs = nullptr;
	};
	class ComputeShader : public Shader
	{
		friend struct ShaderProgram;

	public:
		ComputeShader(ID3D11Device* device, ShaderBlob const& blob) : Shader(blob, EShaderStage::CS)
		{
			HRESULT hr;
			ShaderBlob const& cs_blob = GetBlob();
			hr = device->CreateComputeShader(cs_blob.GetPointer(), cs_blob.GetLength(), nullptr, cs.GetAddressOf());
			BREAK_IF_FAILED(hr);
		}
		void Bind(ID3D11DeviceContext* context)
		{
			context->CSSetShader(cs.Get(), nullptr, 0);
		}
		void Unbind(ID3D11DeviceContext* context)
		{
			context->CSSetShader(nullptr, nullptr, 0);
		}
	private:
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> cs = nullptr;
	};

	struct ShaderProgram
	{
		virtual ~ShaderProgram() = default;
		virtual void Bind(ID3D11DeviceContext* context) = 0;
		virtual void Unbind(ID3D11DeviceContext* context) = 0; 
	};

	struct StandardProgram : ShaderProgram
	{
		VertexShader vs;
		PixelShader ps;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> il = nullptr;

		StandardProgram(ID3D11Device* device, ShaderBlob const& vs_blob, ShaderBlob const& ps_blob,
			bool input_layout_from_reflection = true);

		virtual void Bind(ID3D11DeviceContext* context) override;
		virtual void Unbind(ID3D11DeviceContext* context) override;
	};
	struct TessellationProgram : ShaderProgram
	{
		VertexShader	vs;
		HullShader		hs;
		DomainShader 	ds;
		PixelShader		ps;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	il = nullptr;
		
		TessellationProgram(ID3D11Device* device, ShaderBlob const& vs_blob,
			ShaderBlob const& hs_blob, ShaderBlob const& ds_blob, ShaderBlob const& ps_blob,
			bool input_layout_from_reflection = true);
		virtual void Bind(ID3D11DeviceContext* context) override;
		virtual void Unbind(ID3D11DeviceContext* context) override;
	};
	struct GeometryProgram : ShaderProgram
	{
		VertexShader	vs;
		GeometryShader	gs;
		PixelShader		ps;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>		il = nullptr;

		GeometryProgram(ID3D11Device* device, ShaderBlob const& vs_blob, ShaderBlob const& gs_blob, ShaderBlob const& ps_blob,
			bool input_layout_from_reflection = true);
		virtual void Bind(ID3D11DeviceContext* context) override;
		virtual void Unbind(ID3D11DeviceContext* context) override;
	};
	struct ComputeProgram : ShaderProgram
	{
		ComputeShader cs;

		ComputeProgram(ID3D11Device* device, ShaderBlob const& cs_blob);
		virtual void Bind(ID3D11DeviceContext* context) override;
		virtual void Unbind(ID3D11DeviceContext* context) override;
	};

}