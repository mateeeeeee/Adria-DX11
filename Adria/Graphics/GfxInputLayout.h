#pragma once
#include <string>
#include <vector>
#include "GfxFormat.h"

namespace adria
{
	enum class GfxInputClassification
	{
		PerVertexData,
		PerInstanceData
	};
	struct GfxInputLayoutDesc
	{
		static constexpr uint32 APPEND_ALIGNED_ELEMENT = ~0u;
		struct GfxInputElement
		{
			std::string semantic_name;
			uint32 semantic_index = 0;
			GfxFormat format = GfxFormat::UNKNOWN;
			uint32 input_slot = 0;
			uint32 aligned_byte_offset = APPEND_ALIGNED_ELEMENT;
			GfxInputClassification input_slot_class = GfxInputClassification::PerVertexData;
		};
		std::vector<GfxInputElement> elements;
	};

	class GfxDevice;
	struct GfxShaderBytecode;
	class GfxInputLayout
	{
	public:
		GfxInputLayout(GfxDevice* gfx, GfxShaderBytecode const& vs_blob);
		GfxInputLayout(GfxDevice* gfx, GfxShaderBytecode const& vs_blob, GfxInputLayoutDesc const& desc);

		operator ID3D11InputLayout* () const
		{
			return input_layout.Get();
		}
	private:
		Ref<ID3D11InputLayout> input_layout;
	};
}