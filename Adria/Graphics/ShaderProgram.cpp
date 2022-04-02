#include "ShaderProgram.h"
#include "../Core/Definitions.h" 
#include "../Core/Macros.h" 

namespace adria
{
	StandardProgram::StandardProgram(ID3D11Device* device, ShaderBlob const& vs_blob, ShaderBlob const& ps_blob, bool input_layout_from_reflection)
	{
		HRESULT hr;
		hr = device->CreateVertexShader(vs_blob.GetPointer(), vs_blob.GetLength(), nullptr, vs.GetAddressOf());
		BREAK_IF_FAILED(hr);
		hr = device->CreatePixelShader(ps_blob.GetPointer(), ps_blob.GetLength(), nullptr, ps.GetAddressOf());
		BREAK_IF_FAILED(hr);
		if (input_layout_from_reflection)
		{
			ShaderCompiler::CreateInputLayoutWithReflection(device, vs_blob, il.GetAddressOf());
		}
	}

	void StandardProgram::Bind(ID3D11DeviceContext* context)
	{
		context->IASetInputLayout(il.Get());
		context->VSSetShader(vs.Get(), nullptr, 0);
		context->PSSetShader(ps.Get(), nullptr, 0);
	}


	void StandardProgram::Unbind(ID3D11DeviceContext* context)
	{
		context->IASetInputLayout(nullptr);
		context->VSSetShader(nullptr, nullptr, 0);
		context->PSSetShader(nullptr, nullptr, 0);
	}

	TessellationProgram::TessellationProgram(ID3D11Device* device, ShaderBlob const& vs_blob, ShaderBlob const& hs_blob, ShaderBlob const& ds_blob, ShaderBlob const& ps_blob, bool input_layout_from_reflection /*= true*/)
	{
		HRESULT hr;
		hr = device->CreateVertexShader(vs_blob.GetPointer(), vs_blob.GetLength(), nullptr, vs.GetAddressOf());
		BREAK_IF_FAILED(hr);
		hr = device->CreatePixelShader(ps_blob.GetPointer(), ps_blob.GetLength(), nullptr, ps.GetAddressOf());
		BREAK_IF_FAILED(hr);
		hr = device->CreateHullShader(hs_blob.GetPointer(), hs_blob.GetLength(), nullptr, hs.GetAddressOf());
		BREAK_IF_FAILED(hr);
		hr = device->CreateDomainShader(ds_blob.GetPointer(), ds_blob.GetLength(), nullptr, ds.GetAddressOf());
		BREAK_IF_FAILED(hr);

		if (input_layout_from_reflection)
		{
			ShaderCompiler::CreateInputLayoutWithReflection(device, vs_blob, il.GetAddressOf());
		}
	}
	void TessellationProgram::Bind(ID3D11DeviceContext* context)
	{
		context->IASetInputLayout(il.Get());
		context->VSSetShader(vs.Get(), nullptr, 0);
		context->HSSetShader(hs.Get(), nullptr, 0);
		context->DSSetShader(ds.Get(), nullptr, 0);
		context->PSSetShader(ps.Get(), nullptr, 0);
	}
	void TessellationProgram::Unbind(ID3D11DeviceContext* context)
	{
		context->IASetInputLayout(nullptr);
		context->VSSetShader(nullptr, nullptr, 0);
		context->HSSetShader(nullptr, nullptr, 0);
		context->DSSetShader(nullptr, nullptr, 0);
		context->PSSetShader(nullptr, nullptr, 0);
	}
	GeometryProgram::GeometryProgram(ID3D11Device* device, ShaderBlob const& vs_blob, ShaderBlob const& gs_blob, ShaderBlob const& ps_blob, bool input_layout_from_reflection)
	{
		HRESULT hr;
		hr = device->CreateVertexShader(vs_blob.GetPointer(), vs_blob.GetLength(), nullptr, vs.GetAddressOf());
		BREAK_IF_FAILED(hr);
		hr = device->CreateGeometryShader(gs_blob.GetPointer(), gs_blob.GetLength(), nullptr, gs.GetAddressOf());
		BREAK_IF_FAILED(hr);
		hr = device->CreatePixelShader(ps_blob.GetPointer(), ps_blob.GetLength(), nullptr, ps.GetAddressOf());
		BREAK_IF_FAILED(hr);
		if (input_layout_from_reflection)
		{
			ShaderCompiler::CreateInputLayoutWithReflection(device, vs_blob, il.GetAddressOf());
		}
	}
	void GeometryProgram::Bind(ID3D11DeviceContext* context)
	{
		context->IASetInputLayout(il.Get());
		context->VSSetShader(vs.Get(), nullptr, 0);
		context->GSSetShader(gs.Get(), nullptr, 0);
		context->PSSetShader(ps.Get(), nullptr, 0);
	}
	void GeometryProgram::Unbind(ID3D11DeviceContext* context)
	{
		context->IASetInputLayout(nullptr);
		context->VSSetShader(nullptr, nullptr, 0);
		context->PSSetShader(nullptr, nullptr, 0);
		context->GSSetShader(nullptr, nullptr, 0);
	}
	ComputeProgram::ComputeProgram(ID3D11Device* device, ShaderBlob const& cs_blob)
	{
		HRESULT hr = device->CreateComputeShader(cs_blob.GetPointer(), cs_blob.GetLength(), nullptr, cs.GetAddressOf());
		BREAK_IF_FAILED(hr);
	}
	void ComputeProgram::Bind(ID3D11DeviceContext* context)
	{
		context->CSSetShader(cs.Get(), nullptr, 0);
	}
	void ComputeProgram::Unbind(ID3D11DeviceContext* context)
	{
		context->CSSetShader(nullptr, nullptr, 0);
	}
}
