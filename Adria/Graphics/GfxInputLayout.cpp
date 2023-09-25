#include "GfxInputLayout.h"
#include "GfxDevice.h"
#include "GfxShaderCompiler.h"

namespace adria
{
	static inline void ConvertInputLayout(GfxInputLayoutDesc const& input_layout, std::vector<D3D11_INPUT_ELEMENT_DESC>& element_descs)
	{
		element_descs.resize(input_layout.elements.size());
		for (uint32 i = 0; i < element_descs.size(); ++i)
		{
			GfxInputLayoutDesc::GfxInputElement const& element = input_layout.elements[i];
			D3D11_INPUT_ELEMENT_DESC desc{};
			desc.AlignedByteOffset = element.aligned_byte_offset;
			desc.Format = ConvertGfxFormat(element.format);
			desc.InputSlot = element.input_slot;
			switch (element.input_slot_class)
			{
			case GfxInputClassification::PerVertexData:
				desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
				break;
			case GfxInputClassification::PerInstanceData:
				desc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
				break;
			}
			desc.InstanceDataStepRate = 0;
			if (desc.InputSlotClass == D3D11_INPUT_PER_INSTANCE_DATA) desc.InstanceDataStepRate = 1;
			desc.SemanticIndex = element.semantic_index;
			desc.SemanticName = element.semantic_name.c_str();
			element_descs[i] = desc;
		}
	}

	GfxInputLayout::GfxInputLayout(GfxDevice* gfx, GfxShaderBytecode const& vs_blob, GfxInputLayoutDesc const& desc)
	{
		std::vector<D3D11_INPUT_ELEMENT_DESC> d3d11_desc{};
		ConvertInputLayout(desc, d3d11_desc);
		GFX_CHECK_HR(gfx->Device()->CreateInputLayout(d3d11_desc.data(), (uint32)d3d11_desc.size(), vs_blob.GetPointer(), vs_blob.GetLength(), input_layout.GetAddressOf()));
	}

	GfxInputLayout::GfxInputLayout(GfxDevice* gfx, GfxShaderBytecode const& vs_blob)
	{
		GfxShaderCompiler::CreateInputLayoutWithReflection(gfx->Device(), vs_blob, input_layout.GetAddressOf());
	}

}
