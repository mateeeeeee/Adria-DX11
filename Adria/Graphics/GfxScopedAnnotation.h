#pragma once
#include <d3d11_3.h>

namespace adria
{
	class GfxScopedAnnotation
	{
	public:
		GfxScopedAnnotation(ID3DUserDefinedAnnotation* annotation, wchar_t const* name)
			: annotation(annotation)
		{
			if(annotation) annotation->BeginEvent(name);
		}

		~GfxScopedAnnotation()
		{
			if (annotation) annotation->EndEvent();
		}

	private:
		ID3DUserDefinedAnnotation* annotation;
	};

	#define AdriaGfxScopedAnnotation(annotation, name) GfxScopedAnnotation ADRIA_CONCAT(_annotation, __COUNTER__)(annotation, name)
}