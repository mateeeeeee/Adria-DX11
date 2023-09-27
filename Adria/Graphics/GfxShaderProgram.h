#pragma once
#include "GfxShader.h"

namespace adria
{
	class GfxDevice;
	class GfxCommandContext;
	class GfxInputLayout;

	class GfxShader
	{
	public:

		GfxShaderBytecode& GetBytecode() { return bytecode; }
		GfxShaderStage GetStage() const { return stage; }

		virtual ~GfxShader() = default;
		virtual void Recreate(GfxShaderBytecode const& blob) = 0;

	protected:
		GfxDevice* gfx;
		GfxShaderBytecode bytecode;
		GfxShaderStage stage;

	protected:
		GfxShader(GfxDevice* gfx, GfxShaderBytecode const& bytecode, GfxShaderStage stage)
			: gfx(gfx), bytecode(bytecode), stage(stage)
		{}
	};

	class GfxVertexShader final : public GfxShader
	{
	public:
		GfxVertexShader(GfxDevice* gfx, GfxShaderBytecode const& blob);
		virtual void Recreate(GfxShaderBytecode const& blob) override;

		operator ID3D11VertexShader*() const { return vs.Get(); }

	private:
		ArcPtr<ID3D11VertexShader> vs = nullptr;
	};

	class GfxPixelShader final : public GfxShader
	{
	public:
		GfxPixelShader(GfxDevice* gfx, GfxShaderBytecode const& blob);
		virtual void Recreate(GfxShaderBytecode const& blob) override;

		operator ID3D11PixelShader* () const { return ps.Get(); }

	private:
		ArcPtr<ID3D11PixelShader> ps = nullptr;
	};

	class GfxGeometryShader final : public GfxShader
	{
	public:
		GfxGeometryShader(GfxDevice* gfx, GfxShaderBytecode const& blob);
		virtual void Recreate(GfxShaderBytecode const& blob) override;

		operator ID3D11GeometryShader* () const { return gs.Get(); }

	private:
		ArcPtr<ID3D11GeometryShader> gs = nullptr;
	};

	class GfxDomainShader final : public GfxShader
	{
	public:
		GfxDomainShader(GfxDevice* device, GfxShaderBytecode const& blob);
		virtual void Recreate(GfxShaderBytecode const& blob) override;

		operator ID3D11DomainShader* () const { return ds.Get(); }

	private:
		ArcPtr<ID3D11DomainShader> ds = nullptr;
	};

	class GfxHullShader final : public GfxShader
	{
	public:
		GfxHullShader(GfxDevice* gfx, GfxShaderBytecode const& blob);
		virtual void Recreate(GfxShaderBytecode const& blob) override;

		operator ID3D11HullShader* () const { return hs.Get(); }

	private:
		ArcPtr<ID3D11HullShader> hs = nullptr;
	};

	class GfxComputeShader final : public GfxShader
	{
	public:
		GfxComputeShader(GfxDevice* gfx, GfxShaderBytecode const& blob);
		virtual void Recreate(GfxShaderBytecode const& blob) override;

		operator ID3D11ComputeShader* () const { return cs.Get(); }

	private:
		ArcPtr<ID3D11ComputeShader> cs = nullptr;
	};

	struct GfxShaderProgram
	{
		virtual ~GfxShaderProgram() = default;
		virtual void Bind(GfxCommandContext* context) = 0;
		virtual void Unbind(GfxCommandContext* context) = 0;
	};

	class GfxGraphicsShaderProgram final : public GfxShaderProgram
	{
	public:
		GfxGraphicsShaderProgram();
		GfxGraphicsShaderProgram& SetVertexShader(GfxVertexShader* _vs);
		GfxGraphicsShaderProgram& SetPixelShader(GfxPixelShader* _ps);
		GfxGraphicsShaderProgram& SetDomainShader(GfxDomainShader* _ds);
		GfxGraphicsShaderProgram& SetHullShader(GfxHullShader* _hs);
		GfxGraphicsShaderProgram& SetGeometryShader(GfxGeometryShader* _gs);
		GfxGraphicsShaderProgram& SetInputLayout(GfxInputLayout* _il);

		virtual void Bind(GfxCommandContext* context) override;
		virtual void Unbind(GfxCommandContext* context) override;

	private:
		GfxVertexShader* vs = nullptr;
		GfxPixelShader* ps = nullptr;
		GfxHullShader* hs = nullptr;
		GfxDomainShader* ds = nullptr;
		GfxGeometryShader* gs = nullptr;
		GfxInputLayout* input_layout = nullptr;
	};
	class GfxComputeShaderProgram final : public GfxShaderProgram
	{
	public:
		GfxComputeShaderProgram() = default;

		void SetComputeShader(GfxComputeShader* cs);

		virtual void Bind(GfxCommandContext* context) override;
		virtual void Unbind(GfxCommandContext* context) override;

	private:
		GfxComputeShader* shader = nullptr;
	};
}