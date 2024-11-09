#pragma once
#include "GfxShader.h"

namespace adria
{
	
	struct GfxShaderCompileOutput
	{
		GfxShaderBytecode shader_bytecode;
		std::vector<std::string> includes;
		Uint64 hash;
	};
	
	struct GfxInputLayoutDesc;
	namespace GfxShaderCompiler
	{
		Bool CompileShader(GfxShaderDesc const& input, GfxShaderCompileOutput& output);
		void GetBytecodeFromCompiledShader(Char const* filename, GfxShaderBytecode& blob);
		void FillInputLayoutDesc(GfxShaderBytecode const& blob, GfxInputLayoutDesc& input_desc);
	}
}