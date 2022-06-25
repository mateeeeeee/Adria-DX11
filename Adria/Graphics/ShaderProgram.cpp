#include "ShaderProgram.h"
#include "../Core/Definitions.h" 

namespace adria
{
	StandardProgram::StandardProgram(ID3D11Device* device, ShaderBlob const& vs_blob, ShaderBlob const& ps_blob, bool input_layout_from_reflection)
		: vs(device, vs_blob), ps(device, ps_blob)
	{
		if (input_layout_from_reflection) ShaderCompiler::CreateInputLayoutWithReflection(device, vs_blob, il.GetAddressOf());
	}
	void StandardProgram::Bind(ID3D11DeviceContext* context)
	{
		context->IASetInputLayout(il.Get());
		vs.Bind(context);
		ps.Bind(context);
	}
	void StandardProgram::Unbind(ID3D11DeviceContext* context)
	{
		context->IASetInputLayout(nullptr);
		vs.Unbind(context);
		ps.Unbind(context);
	}

	TessellationProgram::TessellationProgram(ID3D11Device* device, ShaderBlob const& vs_blob, ShaderBlob const& hs_blob, ShaderBlob const& ds_blob, ShaderBlob const& ps_blob, bool input_layout_from_reflection /*= true*/)
		: vs(device, vs_blob), hs(device, hs_blob), ds(device, ds_blob), ps(device, ps_blob)
	{
		if (input_layout_from_reflection) ShaderCompiler::CreateInputLayoutWithReflection(device, vs_blob, il.GetAddressOf());
	}
	void TessellationProgram::Bind(ID3D11DeviceContext* context)
	{
		context->IASetInputLayout(il.Get());
		vs.Bind(context);
		hs.Bind(context);
		ds.Bind(context);
		ps.Bind(context);
	}
	void TessellationProgram::Unbind(ID3D11DeviceContext* context)
	{
		context->IASetInputLayout(nullptr);
		vs.Unbind(context);
		hs.Unbind(context);
		ds.Unbind(context);
		ps.Unbind(context);
	}
	GeometryProgram::GeometryProgram(ID3D11Device* device, ShaderBlob const& vs_blob, ShaderBlob const& gs_blob, ShaderBlob const& ps_blob, bool input_layout_from_reflection)
		: vs(device, vs_blob), gs(device, gs_blob), ps(device, ps_blob)
	{
		if (input_layout_from_reflection) ShaderCompiler::CreateInputLayoutWithReflection(device, vs_blob, il.GetAddressOf());
	}
	void GeometryProgram::Bind(ID3D11DeviceContext* context)
	{
		context->IASetInputLayout(il.Get());
		vs.Bind(context);
		gs.Bind(context);
		ps.Bind(context);
	}
	void GeometryProgram::Unbind(ID3D11DeviceContext* context)
	{
		context->IASetInputLayout(nullptr);
		vs.Unbind(context);
		gs.Unbind(context);
		ps.Unbind(context);
	}
	ComputeProgram::ComputeProgram(ID3D11Device* device, ShaderBlob const& cs_blob)
		: cs(device, cs_blob)
	{
	}
	void ComputeProgram::Bind(ID3D11DeviceContext* context)
	{
		cs.Bind(context);
	}
	void ComputeProgram::Unbind(ID3D11DeviceContext* context)
	{
		cs.Unbind(context);
	}
}
