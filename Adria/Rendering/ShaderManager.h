#pragma once
#include "Enums.h"


namespace adria
{
	struct GfxShaderProgram;
	class GfxDevice;

	namespace ShaderManager
	{
		void Initialize(GfxDevice* device);
		void Destroy();
		GfxShaderProgram* GetShaderProgram(ShaderProgram shader_program);
		void CheckIfShadersHaveChanged();
	};
}