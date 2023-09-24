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
	
	namespace GfxShaderCompiler
	{
		void CompileShader(GfxShaderDesc const& input, GfxShaderCompileOutput& output);
		void GetBytecodeFromCompiledShader(char const* filename, GfxShaderBytecode& blob);
		void CreateInputLayoutWithReflection(ID3D11Device* device, GfxShaderBytecode const& blob, ID3D11InputLayout** il);
	}
}