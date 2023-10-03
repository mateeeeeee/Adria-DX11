#pragma once
#include "GfxShader.h"

namespace adria
{
	
	struct GfxShaderCompileOutput
	{
		GfxShaderBytecode shader_bytecode;
		std::vector<std::string> includes;
		uint64 hash;
	};
	
	struct GfxInputLayoutDesc;
	namespace GfxShaderCompiler
	{
		bool CompileShader(GfxShaderDesc const& input, GfxShaderCompileOutput& output);
		void GetBytecodeFromCompiledShader(char const* filename, GfxShaderBytecode& blob);
		void FillInputLayoutDesc(GfxShaderBytecode const& blob, GfxInputLayoutDesc& input_desc);
	}
}