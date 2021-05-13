#pragma once
#include <wrl.h>
#include "ShaderUtility.h"


namespace adria
{
	struct ShaderProgram 
	{
		virtual ~ShaderProgram() = default;
		virtual void Bind(ID3D11DeviceContext* context) = 0;
		virtual void Unbind(ID3D11DeviceContext* context) = 0; //this can be static rather than virtual 
		};

	struct StandardProgram : ShaderProgram
	{
		Microsoft::WRL::ComPtr<ID3D11VertexShader> vs	= nullptr;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> ps	= nullptr;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> il	= nullptr;

		void Create(ID3D11Device* device, ShaderBlob const& vs_blob, ShaderBlob const& ps_blob,
			bool input_layout_from_reflection = true);

		virtual void Bind(ID3D11DeviceContext* context) override;

		virtual void Unbind(ID3D11DeviceContext* context) override;
	};

	struct TessellationProgram : ShaderProgram
	{
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	vs = nullptr;;
		Microsoft::WRL::ComPtr<ID3D11HullShader>	hs = nullptr;;
		Microsoft::WRL::ComPtr<ID3D11DomainShader>	ds = nullptr;;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	ps = nullptr;;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	il = nullptr;;
		
		void Create(ID3D11Device* device, ShaderBlob const& vs_blob, ShaderBlob const& ps_blob,
			ShaderBlob const& hs_blob, ShaderBlob const& ds_blob,
			bool input_layout_from_reflection = true);


		virtual void Bind(ID3D11DeviceContext* context) override;

		virtual void Unbind(ID3D11DeviceContext* context) override;

		
	};

	struct GeometryProgram : ShaderProgram
	{
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		vs= nullptr;
		Microsoft::WRL::ComPtr<ID3D11GeometryShader>	gs= nullptr;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		ps= nullptr;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>		il= nullptr;

		void Create(ID3D11Device* device, ShaderBlob const& vs_blob, ShaderBlob const& gs_blob, ShaderBlob const& ps_blob,
			bool input_layout_from_reflection = true);
		
		virtual void Bind(ID3D11DeviceContext* context) override;

		virtual void Unbind(ID3D11DeviceContext* context) override;
	};

	struct ComputeProgram : ShaderProgram
	{
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> cs = nullptr;

		void Create(ID3D11Device* device, ShaderBlob const& cs_blob);

		virtual void Bind(ID3D11DeviceContext* context) override;

		virtual void Unbind(ID3D11DeviceContext* context) override;
	};

}