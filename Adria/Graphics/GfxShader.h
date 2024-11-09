#pragma once
#include <string>
#include <vector>

namespace adria
{
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
	using GfxShaderBlob = std::vector<Uint8>;

	struct GfxShaderBytecode
	{
		GfxShaderBlob bytecode;

		void SetBytecode(void* data, Uint32 data_size)
		{
			bytecode.resize(data_size);
			memcpy(bytecode.data(), data, data_size);
		}
		void* GetPointer() const
		{
			return (void*)bytecode.data();
		}
		Uint32 GetLength() const
		{
			return (Uint32)bytecode.size();
		}
	};

	enum GfxShaderCompilerFlagBit
	{
		GfxShaderCompilerFlagBit_None = 0,
		GfxShaderCompilerFlagBit_Debug = 1 << 0,
		GfxShaderCompilerFlagBit_DisableOptimization = 1 << 1,
	};

	struct GfxShaderDesc
	{
		GfxShaderStage stage = GfxShaderStage::StageCount;
		std::string source_file = "";
		std::string entrypoint = "";
		std::vector<GfxShaderMacro> macros;
		Uint64 flags = GfxShaderCompilerFlagBit_None;
	};
}