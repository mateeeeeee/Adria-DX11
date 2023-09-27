#include <d3dcompiler.h> 
#include "GfxShaderCompiler.h"
#include "GfxInputLayout.h"
#include "Utilities/HashUtil.h"
#include "Utilities/StringUtil.h"
#include "Utilities/FilesUtil.h"
#include "Core/Defines.h" 
#include "Logging/Logger.h" 
#include "cereal/archives/binary.hpp"

namespace adria
{
	namespace 
	{
		char const* shaders_cache_directory = "Resources/ShaderCache/";
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

		bool CheckCache(char const* cache_path, GfxShaderDesc const& input, GfxShaderCompileOutput& output)
		{
			if (!FileExists(cache_path)) return false;
			if (GetFileLastWriteTime(cache_path) < GetFileLastWriteTime(input.source_file)) return false;

			std::ifstream is(cache_path, std::ios::binary);
			cereal::BinaryInputArchive archive(is);

			archive(output.hash);
			archive(output.includes);
			uint32 binary_size = 0;
			archive(binary_size);
			std::unique_ptr<char[]> binary_data(new char[binary_size]);
			archive.loadBinary(binary_data.get(), binary_size);
			output.shader_bytecode.SetBytecode(binary_data.get(), binary_size);
			return true;
		}
		bool SaveToCache(char const* cache_path, GfxShaderCompileOutput const& output)
		{
			std::ofstream os(cache_path, std::ios::binary);
			cereal::BinaryOutputArchive archive(os);
			archive(output.hash);
			archive(output.includes);
			archive(output.shader_bytecode.GetLength());
			archive.saveBinary(output.shader_bytecode.GetPointer(), output.shader_bytecode.GetLength());
			return true;
		}
	}

	namespace GfxShaderCompiler
	{

		void GetBytecodeFromCompiledShader(char const* filename, GfxShaderBytecode& blob)
		{
			ArcPtr<ID3DBlob> bytecode_blob;

			std::wstring wide_filename = ToWideString(std::string(filename));
			HRESULT hr = D3DReadFileToBlob(wide_filename.c_str(), bytecode_blob.GetAddressOf());
			GFX_CHECK_HR(hr);

			blob.bytecode.resize(bytecode_blob->GetBufferSize());
			std::memcpy(blob.GetPointer(), bytecode_blob->GetBufferPointer(), blob.GetLength());
		}

		void CompileShader(GfxShaderDesc const& input,
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

			std::string macro_key;
			for (GfxShaderMacro const& macro : input.macros)
			{
				macro_key += macro.name;
				macro_key += macro.value;
			}
			uint64 macro_hash = crc64(macro_key.c_str(), macro_key.size());

			std::string build_string = input.flags & GfxShaderCompilerFlagBit_Debug ? "debug" : "release";
			char cache_path[256];
			sprintf_s(cache_path, "%s%s_%s_%llx_%s.bin", shaders_cache_directory,
				GetFilenameWithoutExtension(input.source_file).c_str(), entrypoint.c_str(), macro_hash, build_string.c_str());

			if (CheckCache(cache_path, input, output)) return;
			ADRIA_LOG(INFO, "Shader '%s.%s' not found in cache. Compiling...", input.source_file.c_str(), entrypoint.c_str());

			uint32 shader_compile_flags = D3DCOMPILE_ENABLE_STRICTNESS;
			if (input.flags & GfxShaderCompilerFlagBit_DisableOptimization) shader_compile_flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
			if (input.flags & GfxShaderCompilerFlagBit_Debug) shader_compile_flags |= D3DCOMPILE_DEBUG;
			std::vector<D3D_SHADER_MACRO> defines{};
			defines.resize(input.macros.size());

			for (uint32 i = 0; i < input.macros.size(); ++i)
			{
				defines[i].Name = (char*)malloc(sizeof(input.macros[i].name));
				defines[i].Definition = (char*)malloc(sizeof(input.macros[i].value));

				strcpy(const_cast<char*>(defines[i].Name), input.macros[i].name.c_str());
				strcpy(const_cast<char*>(defines[i].Definition), input.macros[i].value.c_str());
			}
			defines.push_back({ NULL,NULL });

			ArcPtr<ID3DBlob> bytecode_blob = nullptr;
			ArcPtr<ID3DBlob> error_blob = nullptr;

			CShaderInclude includer(GetParentPath(input.source_file).c_str());
			HRESULT hr = D3DCompileFromFile(ToWideString(input.source_file).c_str(), defines.data(),
				&includer, entrypoint.c_str(), model.c_str(), shader_compile_flags, 0,
				bytecode_blob.GetAddressOf(), error_blob.GetAddressOf());

			auto const& includes = includer.GetIncludes();
			if (FAILED(hr) && error_blob) OutputDebugStringA(reinterpret_cast<const char*>(error_blob->GetBufferPointer()));
			for (auto& define : defines)
			{
				if (define.Name)		free((void*)define.Name);
				if (define.Definition)  free((void*)define.Definition); //change malloc and free to new and delete
			}

			uint64 shader_hash = crc64((char*)bytecode_blob->GetBufferPointer(), bytecode_blob->GetBufferSize());

			output.shader_bytecode.bytecode.resize(bytecode_blob->GetBufferSize());
			std::memcpy(output.shader_bytecode.GetPointer(), bytecode_blob->GetBufferPointer(), bytecode_blob->GetBufferSize());
			output.includes = includes;
			output.includes.push_back(input.source_file);
			output.hash = shader_hash;
			SaveToCache(cache_path, output);
		}


		void FillInputLayoutDesc(GfxShaderBytecode const& blob, GfxInputLayoutDesc& input_desc)
		{
			ArcPtr<ID3D11ShaderReflection> vertex_shader_reflection = nullptr;
			GFX_CHECK_HR(D3DReflect(blob.GetPointer(), blob.GetLength(), IID_ID3D11ShaderReflection, (void**)vertex_shader_reflection.GetAddressOf()));

			D3D11_SHADER_DESC shaderDesc;
			vertex_shader_reflection->GetDesc(&shaderDesc);

			input_desc.elements.clear();
			input_desc.elements.resize(shaderDesc.InputParameters);
			for (uint32 i = 0; i < shaderDesc.InputParameters; i++)
			{
				D3D11_SIGNATURE_PARAMETER_DESC param_desc;
				vertex_shader_reflection->GetInputParameterDesc(i, &param_desc);

				input_desc.elements[i].semantic_name = param_desc.SemanticName;
				input_desc.elements[i].semantic_index = param_desc.SemanticIndex;
				input_desc.elements[i].input_slot = 0;
				input_desc.elements[i].aligned_byte_offset = D3D11_APPEND_ALIGNED_ELEMENT;
				input_desc.elements[i].input_slot_class = GfxInputClassification::PerVertexData;

				if (param_desc.Mask == 1)
				{
					if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_desc.elements[i].format = GfxFormat::R32_UINT;
					else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) input_desc.elements[i].format = GfxFormat::R32_SINT;
					else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_desc.elements[i].format = GfxFormat::R32_FLOAT;
				}
				else if (param_desc.Mask <= 3)
				{
					if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_desc.elements[i].format = GfxFormat::R32G32_UINT;
					else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) input_desc.elements[i].format = GfxFormat::R32G32_SINT;
					else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_desc.elements[i].format = GfxFormat::R32G32_FLOAT;
				}
				else if (param_desc.Mask <= 7)
				{
					if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_desc.elements[i].format = GfxFormat::R32G32B32_UINT;
					else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) input_desc.elements[i].format = GfxFormat::R32G32B32_SINT;
					else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_desc.elements[i].format = GfxFormat::R32G32B32_FLOAT;
				}
				else if (param_desc.Mask <= 15)
				{
					if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) input_desc.elements[i].format = GfxFormat::R32G32B32A32_UINT;
					else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) input_desc.elements[i].format = GfxFormat::R32G32B32A32_SINT;
					else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) input_desc.elements[i].format = GfxFormat::R32G32B32A32_FLOAT;
				}
			}
		}
	}
}