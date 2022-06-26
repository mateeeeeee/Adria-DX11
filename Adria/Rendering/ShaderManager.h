#pragma once
#include "Enums.h"

struct ID3D11Device;

namespace adria
{
	struct ShaderProgram;

	namespace ShaderManager
	{
		void Initialize(ID3D11Device* device);
		void Destroy();
		ShaderProgram* GetShaderProgram(EShaderProgram shader_program);
		void CheckIfShadersHaveChanged();
	};
}