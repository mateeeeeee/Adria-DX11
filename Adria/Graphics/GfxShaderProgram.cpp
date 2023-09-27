#include "GfxShaderProgram.h"
#include "GfxDevice.h"
#include "GfxCommandContext.h"

namespace adria
{

	GfxVertexShader::GfxVertexShader(GfxDevice* gfx, GfxShaderBytecode const& blob) : GfxShader(gfx, blob, GfxShaderStage::VS)
	{
		GfxShaderBytecode const& vs_blob = GetBytecode();
		HRESULT hr = gfx->GetDevice()->CreateVertexShader(vs_blob.GetPointer(), vs_blob.GetLength(), nullptr, vs.GetAddressOf());
		GFX_CHECK_HR(hr);
	}
	void GfxVertexShader::Recreate(GfxShaderBytecode const& blob)
	{
		GetBytecode() = blob;
		HRESULT hr = gfx->GetDevice()->CreateVertexShader(blob.GetPointer(), blob.GetLength(), nullptr, vs.ReleaseAndGetAddressOf());
		GFX_CHECK_HR(hr);
	}

	GfxPixelShader::GfxPixelShader(GfxDevice* gfx, GfxShaderBytecode const& blob) : GfxShader(gfx, blob, GfxShaderStage::PS)
	{
		GfxShaderBytecode const& ps_blob = GetBytecode();
		HRESULT hr = gfx->GetDevice()->CreatePixelShader(ps_blob.GetPointer(), ps_blob.GetLength(), nullptr, ps.GetAddressOf());
		GFX_CHECK_HR(hr);
	}
	void GfxPixelShader::Recreate(GfxShaderBytecode const& blob)
	{
		GetBytecode() = blob;
		HRESULT hr = gfx->GetDevice()->CreatePixelShader(blob.GetPointer(), blob.GetLength(), nullptr, ps.ReleaseAndGetAddressOf());
		GFX_CHECK_HR(hr);
	}

	GfxGeometryShader::GfxGeometryShader(GfxDevice* gfx, GfxShaderBytecode const& blob) : GfxShader(gfx, blob, GfxShaderStage::GS)
	{
		GfxShaderBytecode const& gs_blob = GetBytecode();
		HRESULT hr = gfx->GetDevice()->CreateGeometryShader(gs_blob.GetPointer(), gs_blob.GetLength(), nullptr, gs.GetAddressOf());
		GFX_CHECK_HR(hr);
	}
	void GfxGeometryShader::Recreate(GfxShaderBytecode const& blob)
	{
		GetBytecode() = blob;
		HRESULT hr = gfx->GetDevice()->CreateGeometryShader(blob.GetPointer(), blob.GetLength(), nullptr, gs.ReleaseAndGetAddressOf());
		GFX_CHECK_HR(hr);
	}

	GfxDomainShader::GfxDomainShader(GfxDevice* gfx, GfxShaderBytecode const& blob) : GfxShader(gfx, blob, GfxShaderStage::DS)
	{
		GfxShaderBytecode const& ds_blob = GetBytecode();
		HRESULT hr = gfx->GetDevice()->CreateDomainShader(ds_blob.GetPointer(), ds_blob.GetLength(), nullptr, ds.GetAddressOf());
		GFX_CHECK_HR(hr);
	}
	void GfxDomainShader::Recreate(GfxShaderBytecode const& blob)
	{
		GetBytecode() = blob;
		HRESULT hr = gfx->GetDevice()->CreateDomainShader(blob.GetPointer(), blob.GetLength(), nullptr, ds.ReleaseAndGetAddressOf());
		GFX_CHECK_HR(hr);
	}

	GfxHullShader::GfxHullShader(GfxDevice* gfx, GfxShaderBytecode const& blob) : GfxShader(gfx, blob, GfxShaderStage::HS)
	{
		GfxShaderBytecode const& hs_blob = GetBytecode();
		HRESULT hr = gfx->GetDevice()->CreateHullShader(hs_blob.GetPointer(), hs_blob.GetLength(), nullptr, hs.GetAddressOf());
		GFX_CHECK_HR(hr);
	}
	void GfxHullShader::Recreate(GfxShaderBytecode const& blob)
	{
		GetBytecode() = blob;
		HRESULT hr = gfx->GetDevice()->CreateHullShader(blob.GetPointer(), blob.GetLength(), nullptr, hs.ReleaseAndGetAddressOf());
		GFX_CHECK_HR(hr);
	}

	GfxComputeShader::GfxComputeShader(GfxDevice* gfx, GfxShaderBytecode const& blob) : GfxShader(gfx, blob, GfxShaderStage::CS)
	{
		GfxShaderBytecode const& cs_blob = GetBytecode();
		HRESULT hr = gfx->GetDevice()->CreateComputeShader(cs_blob.GetPointer(), cs_blob.GetLength(), nullptr, cs.GetAddressOf());
		GFX_CHECK_HR(hr);
	}
	void GfxComputeShader::Recreate(GfxShaderBytecode const& blob)
	{
		GetBytecode() = blob;
		HRESULT hr = gfx->GetDevice()->CreateComputeShader(blob.GetPointer(), blob.GetLength(), nullptr, cs.ReleaseAndGetAddressOf());
		GFX_CHECK_HR(hr);
	}

	GfxGraphicsShaderProgram::GfxGraphicsShaderProgram()
	{

	}
	GfxGraphicsShaderProgram& GfxGraphicsShaderProgram::SetVertexShader(GfxVertexShader* _vs)
	{
		vs = _vs;
		return *this;
	}
	GfxGraphicsShaderProgram& GfxGraphicsShaderProgram::SetPixelShader(GfxPixelShader* _ps)
	{
		ps = _ps;
		return *this;
	}
	GfxGraphicsShaderProgram& GfxGraphicsShaderProgram::SetDomainShader(GfxDomainShader* _ds)
	{
		ds = _ds;
		return *this;
	}
	GfxGraphicsShaderProgram& GfxGraphicsShaderProgram::SetHullShader(GfxHullShader* _hs)
	{
		hs = _hs;
		return *this;
	}
	GfxGraphicsShaderProgram& GfxGraphicsShaderProgram::SetGeometryShader(GfxGeometryShader* _gs)
	{
		gs = _gs;
		return *this;
	}
	GfxGraphicsShaderProgram& GfxGraphicsShaderProgram::SetInputLayout(GfxInputLayout* _il)
	{
		input_layout = _il;
		return *this;
	}

	void GfxGraphicsShaderProgram::Bind(GfxCommandContext* context)
	{
		if (input_layout) context->SetInputLayout(input_layout);
		if (vs) context->SetVertexShader(vs);
		if (gs) context->SetGeometryShader(gs);
		if (hs) context->SetHullShader(hs);
		if (ds) context->SetDomainShader(ds);
		if (ps) context->SetPixelShader(ps);
	}
	void GfxGraphicsShaderProgram::Unbind(GfxCommandContext* context)
	{
		if (input_layout) context->SetInputLayout(nullptr);
		if (vs) context->SetVertexShader(nullptr);
		if (gs) context->SetGeometryShader(nullptr);
		if (hs) context->SetHullShader(nullptr);
		if (ds) context->SetDomainShader(nullptr);
		if (ps) context->SetPixelShader(nullptr);
	}


	void GfxComputeShaderProgram::SetComputeShader(GfxComputeShader* cs)
	{
		shader = cs;
	}
	void GfxComputeShaderProgram::Bind(GfxCommandContext* context)
	{
		if (shader) context->SetComputeShader(shader);
	}
	void GfxComputeShaderProgram::Unbind(GfxCommandContext* context)
	{
		if (shader) context->SetComputeShader(nullptr);
	}

}