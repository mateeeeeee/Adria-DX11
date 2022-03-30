#pragma once
#include <string>
#include <vector>
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

	struct ShaderDefine
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
		STAGE_COUNT
	};

	struct ShaderInfo
	{
		enum FLAGS
		{
			FLAG_NONE = 0,
			FLAG_DEBUG = 1 << 0,
			FLAG_DISABLE_OPTIMIZATION = 1 << 1,
		};

		EShaderStage stage = EShaderStage::STAGE_COUNT;
		std::string shadersource = "";
		std::string entrypoint = "";
		std::vector<ShaderDefine> defines;
		uint64 flags = FLAG_NONE;
	};
	
	namespace ShaderUtility
	{
		void CompileShader(ShaderInfo const& input, ShaderBlob& blob);

		void GetBlobFromCompiledShader(std::string_view filename, ShaderBlob& blob);

		void CreateInputLayoutWithReflection(ID3D11Device* device, ShaderBlob const& blob, ID3D11InputLayout** il);
	}
}