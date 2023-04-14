#pragma once
#include "Enums.h"

struct ID3D11Device;

namespace adria
{
	struct GfxShaderProgram;

	namespace ShaderManager
	{
		void Initialize(ID3D11Device* device);
		void Destroy();
		GfxShaderProgram* GetShaderProgram(EShaderProgram shader_program);
		void CheckIfShadersHaveChanged();
	};
}