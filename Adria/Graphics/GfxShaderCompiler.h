#pragma once
#include <string>
#include <vector>
#include <span>
#include <d3d11.h>
#include "Core/CoreTypes.h" 

namespace adria
{
	struct GfxShaderBlob
	{
		std::vector<uint8> bytecode;

		void SetBytecode(void* data, size_t data_size)
		{
			bytecode.resize(data_size);
			memcpy(bytecode.data(), data, data_size);
		}
		void* GetPointer() const
		{
			return (void*)bytecode.data();
		}
		size_t GetLength() const
		{
			return bytecode.size();
		}
	};
	struct GfxShaderMacro
	{
		std::string name;
		std::string value;
	};
	enum class GfxShaderStage
	{
		VS,
		PS,
		HS,
		DS,
		GS,
		CS,
		StageCount
	};

	struct GfxShaderCompileInput
	{
		enum GfxShaderCompileFlags
		{
			FlagNone = 0,
			FlagDebug = 1 << 0,
			FlagDisableOptimization = 1 << 1,
		};
		GfxShaderStage stage = GfxShaderStage::StageCount;
		std::string source_file = "";
		std::string entrypoint = "";
		std::vector<GfxShaderMacro> macros;
		uint64 flags = FlagNone;
	};

	struct GfxShaderCompileOutput
	{
		GfxShaderBlob blob;
		std::vector<std::string> includes;
		uint64 hash;
	};
	
	namespace GfxShaderCompiler
	{
		void CompileShader(GfxShaderCompileInput const& input, GfxShaderCompileOutput& output);
		void GetBlobFromCompiledShader(char const* filename, GfxShaderBlob& blob);
		void CreateInputLayoutWithReflection(ID3D11Device* device, GfxShaderBlob const& blob, ID3D11InputLayout** il);
	}
}