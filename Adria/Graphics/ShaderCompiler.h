#pragma once
#include <string>
#include <vector>
#include <span>
#include <d3d11.h>
#include "../Core/Definitions.h" 

namespace adria
{
	struct ShaderBlob
	{
		std::vector<uint8> bytecode;

		void* GetPointer() const
		{
			return (void*)bytecode.data();
		}

		size_t GetLength() const
		{
			return bytecode.size();
		}

	};
	struct ShaderMacro
	{
		std::string name;
		std::string definition;
	};
	enum class EShaderStage
	{
		VS,
		PS,
		HS,
		DS,
		GS,
		CS,
		StageCount
	};

	struct ShaderCompileInput
	{
		enum EShaderCompileFlags
		{
			FlagNone = 0,
			FlagDebug = 1 << 0,
			FlagDisableOptimization = 1 << 1,
		};
		EShaderStage stage = EShaderStage::StageCount;
		std::string source_file = "";
		std::string entrypoint = "";
		std::vector<ShaderMacro> macros;
		uint64 flags = FlagNone;
	};

	struct ShaderCompileOutput
	{
		ShaderBlob blob;
		std::vector<std::string> include_files;
	};
	
	namespace ShaderCompiler
	{
		void CompileShader(ShaderCompileInput const& input, ShaderBlob& blob);
		void GetBlobFromCompiledShader(char const* filename, ShaderBlob& blob);
		void CreateInputLayoutWithReflection(ID3D11Device* device, ShaderBlob const& blob, ID3D11InputLayout** il);
	}
}