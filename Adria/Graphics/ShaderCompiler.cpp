#include <wrl.h>
#include <d3dcompiler.h> 
#include "ShaderCompiler.h"
#include "../Utilities/HashUtil.h"
#include "../Core/Macros.h" 
#include "../Utilities/StringUtil.h"


namespace adria
{

	namespace ShaderCompiler
	{

		void GetBlobFromCompiledShader(char const* filename, ShaderBlob& blob)
		{
			Microsoft::WRL::ComPtr<ID3DBlob> pBytecodeBlob;

			std::wstring wide_filename = ConvertToWide(std::string(filename));
			HRESULT hr = D3DReadFileToBlob(wide_filename.c_str(), &pBytecodeBlob);
			BREAK_IF_FAILED(hr);

			blob.bytecode.resize(pBytecodeBlob->GetBufferSize());
			std::memcpy(blob.GetPointer(), pBytecodeBlob->GetBufferPointer(), blob.GetLength());
		}

		void CompileShader(ShaderInfo const& input,
			ShaderBlob& blob)
		{

			std::string entrypoint, model;
			switch (input.stage)
			{
			case EShaderStage::VS:
				entrypoint = "vs_main";
				model = "vs_5_0";
				break;
			case EShaderStage::PS:
				entrypoint = "ps_main";
				model = "ps_5_0";
				break;
			case EShaderStage::HS:
				entrypoint = "hs_main";
				model = "hs_5_0";
				break;
			case EShaderStage::DS:
				entrypoint = "ds_main";
				model = "ds_5_0";
				break;
			case EShaderStage::GS:
				entrypoint = "gs_main";
				model = "gs_5_0";
				break;
			case EShaderStage::CS:
				entrypoint = "cs_main";
				model = "cs_5_0";
				break;
			default:
				ADRIA_ASSERT(false && "Unsupported Shader Stage!");
			}

			entrypoint = input.entrypoint.empty() ? entrypoint : input.entrypoint;

			UINT shader_compile_flags = D3DCOMPILE_ENABLE_STRICTNESS;
			if (input.flags & ShaderInfo::FLAG_DISABLE_OPTIMIZATION)
			{
				shader_compile_flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
			}

			if (input.flags & ShaderInfo::FLAG_DEBUG)
			{
				shader_compile_flags |= D3DCOMPILE_DEBUG;
			}

			std::vector<D3D_SHADER_MACRO> defines{};
			defines.resize(input.macros.size());
			
			for (uint32_t i = 0; i < input.macros.size(); ++i)
			{
				defines[i].Name = (char*)malloc(sizeof(input.macros[i].name));
				defines[i].Definition = (char*)malloc(sizeof(input.macros[i].definition));

				strcpy(const_cast<char*>(defines[i].Name), input.macros[i].name.c_str());
				strcpy(const_cast<char*>(defines[i].Definition), input.macros[i].definition.c_str());
			}
			defines.push_back({ NULL,NULL });

			Microsoft::WRL::ComPtr<ID3DBlob> pBytecodeBlob = nullptr;
			Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;

			HRESULT hr = D3DCompileFromFile(ConvertToWide(input.shadersource).c_str(), 
				defines.data(),
				D3D_COMPILE_STANDARD_FILE_INCLUDE, entrypoint.c_str(), model.c_str(),
				shader_compile_flags, 0, &pBytecodeBlob, &pErrorBlob);

			if (FAILED(hr) && pErrorBlob) OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));

			for (auto& define : defines)
			{
				if (define.Name)		free((void*)define.Name);
				if (define.Definition)  free((void*)define.Definition); //change malloc and free to new and delete
			}
			BREAK_IF_FAILED(hr);
			blob.bytecode.resize(pBytecodeBlob->GetBufferSize());
			std::memcpy(blob.GetPointer(), pBytecodeBlob->GetBufferPointer(), blob.GetLength());
			pBytecodeBlob->Release();
		}

		void CreateInputLayoutWithReflection(ID3D11Device* device, ShaderBlob const& blob, ID3D11InputLayout** il)
		{
			// Reflect shader info
			Microsoft::WRL::ComPtr<ID3D11ShaderReflection> pVertexShaderReflection = nullptr;
			BREAK_IF_FAILED(D3DReflect(blob.GetPointer(), blob.GetLength(), IID_ID3D11ShaderReflection, (void**)pVertexShaderReflection.GetAddressOf()));

			// Get shader info
			D3D11_SHADER_DESC shaderDesc;
			pVertexShaderReflection->GetDesc(&shaderDesc);

			// Read input layout description from shader info
			std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
			for (uint32_t i = 0; i < shaderDesc.InputParameters; i++)
			{
				D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
				pVertexShaderReflection->GetInputParameterDesc(i, &paramDesc);

				// fill out input element desc
				D3D11_INPUT_ELEMENT_DESC elementDesc{};
				elementDesc.SemanticName = paramDesc.SemanticName;
				elementDesc.SemanticIndex = paramDesc.SemanticIndex;
				elementDesc.InputSlot = 0;
				elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
				elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
				elementDesc.InstanceDataStepRate = 0;

				// determine DXGI format
				if (paramDesc.Mask == 1)
				{
					if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
					else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
					else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
				}
				else if (paramDesc.Mask <= 3)
				{
					if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
					else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
					else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
				}
				else if (paramDesc.Mask <= 7)
				{
					if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
					else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
					else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
				}
				else if (paramDesc.Mask <= 15)
				{
					if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
					else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
					else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				}

				inputLayoutDesc.push_back(elementDesc);
			}

			// Try to create Input Layout
			HRESULT hr = device->CreateInputLayout(inputLayoutDesc.data(), static_cast<UINT>(inputLayoutDesc.size()), blob.GetPointer(), blob.GetLength(), 
				il);
			BREAK_IF_FAILED(hr);
		}

	}
}