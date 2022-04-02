#pragma once
#include "Enums.h"

struct ID3D11Device;

namespace adria
{
	struct ShaderProgram;

	namespace ShaderCache
	{
		void Initialize(ID3D11Device* device);

		void RecompileShader(EShader shader);

		ShaderProgram* GetShaderProgram(EShaderProgram shader_program);

		void RecompileChangedShaders();
	};



}