#include <wrl.h>
#include <d3dcompiler.h> 
#include <fstream> 
#include "GfxShaderCompiler.h"
#include "Utilities/HashUtil.h"
#include "Utilities/StringUtil.h"
#include "Utilities/FilesUtil.h"
#include "Core/Defines.h" 

namespace adria
{

	namespace 
	{
		class CShaderInclude : public ID3DInclude
		{
		public:
			CShaderInclude(char const* shader_dir) : shader_dir(shader_dir)
			{}

			HRESULT __stdcall Open(
				D3D_INCLUDE_TYPE IncludeType,
				LPCSTR pFileName,
				LPCVOID pParentData,
				LPCVOID* ppData,
				UINT* pBytes)
			{
				fs::path final_path;
				switch (IncludeType)
				{
				case D3D_INCLUDE_LOCAL: 
					final_path = fs::path(shader_dir) / fs::path(pFileName);
					break;
				case D3D_INCLUDE_SYSTEM: 
				default:
					ADRIA_ASSERT_MSG(false, "Includer supports only local includes!");
				}
				includes.push_back(final_path.string());
				std::ifstream file_stream(final_path.string());
				if (file_stream)
				{
					std::string contents;
					contents.assign(std::istreambuf_iterator<char>(file_stream),
									std::istreambuf_iterator<char>());

					char* buf = new char[contents.size()];
					contents.copy(buf, contents.size());
					*ppData = buf;
					*pBytes = (UINT)contents.size();
				}
				else
				{
					*ppData = nullptr;
					*pBytes = 0;
				}
				return S_OK;
			}

			HRESULT __stdcall Close(LPCVOID pData)
			{
				char* buf = (char*)pData;
				delete[] buf;
				return S_OK;
			}

			std::vector<std::string> const& GetIncludes() const { return includes; }

		private:
			std::string shader_dir;
			std::vector<std::string> includes;
		};
	}

	namespace GfxShaderCompiler
	{

		void GetBlobFromCompiledShader(char const* filename, GfxShaderBlob& blob)
		{
			Microsoft::WRL::ComPtr<ID3DBlob> pBytecodeBlob;

			std::wstring wide_filename = ToWideString(std::string(filename));
			HRESULT hr = D3DReadFileToBlob(wide_filename.c_str(), &pBytecodeBlob);
			BREAK_IF_FAILED(hr);

			blob.bytecode.resize(pBytecodeBlob->GetBufferSize());
			std::memcpy(blob.GetPointer(), pBytecodeBlob->GetBufferPointer(), blob.GetLength());
		}

		void CompileShader(GfxShaderCompileInput const& input,
			GfxShaderCompileOutput& output)
		{
			output = GfxShaderCompileOutput{};
			std::string entrypoint, model;
			switch (input.stage)
			{
			case GfxShaderStage::VS:
				entrypoint = "vs_main";
				model = "vs_5_0";
				break;
			case GfxShaderStage::PS:
				entrypoint = "ps_main";
				model = "ps_5_0";
				break;
			case GfxShaderStage::HS:
				entrypoint = "hs_main";
				model = "hs_5_0";
				break;
			case GfxShaderStage::DS:
				entrypoint = "ds_main";
				model = "ds_5_0";
				break;
			case GfxShaderStage::GS:
				entrypoint = "gs_main";
				model = "gs_5_0";
				break;
			case GfxShaderStage::CS:
				entrypoint = "cs_main";
				model = "cs_5_0";
				break;
			default:
				ADRIA_ASSERT(false && "Unsupported Shader Stage!");
			}

			entrypoint = input.entrypoint.empty() ? entrypoint : input.entrypoint;

			UINT shader_compile_flags = D3DCOMPILE_ENABLE_STRICTNESS;
			if (input.flags & GfxShaderCompileInput::FlagDisableOptimization)
			{
				shader_compile_flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
			}

			if (input.flags & GfxShaderCompileInput::FlagDebug)
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

			CShaderInclude includer(GetParentPath(input.source_file).c_str());
			HRESULT hr = D3DCompileFromFile(ToWideString(input.source_file).c_str(), 
				defines.data(),
				&includer, entrypoint.c_str(), model.c_str(),
				shader_compile_flags, 0, &pBytecodeBlob, &pErrorBlob);
			auto const& includes = includer.GetIncludes();

			if (FAILED(hr) && pErrorBlob) OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));

			for (auto& define : defines)
			{
				if (define.Name)		free((void*)define.Name);
				if (define.Definition)  free((void*)define.Definition); //change malloc and free to new and delete
			}
			BREAK_IF_FAILED(hr);
			output.blob.bytecode.resize(pBytecodeBlob->GetBufferSize());
			std::memcpy(output.blob.GetPointer(), pBytecodeBlob->GetBufferPointer(), pBytecodeBlob->GetBufferSize());
			output.dependent_files = includes;
			output.dependent_files.push_back(input.source_file);
		}

		void CreateInputLayoutWithReflection(ID3D11Device* device, GfxShaderBlob const& blob, ID3D11InputLayout** il)
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

			if (inputLayoutDesc.empty()) return;
			HRESULT hr = device->CreateInputLayout(inputLayoutDesc.data(), static_cast<UINT>(inputLayoutDesc.size()), blob.GetPointer(), blob.GetLength(), il);
			BREAK_IF_FAILED(hr);
		}

	}
}