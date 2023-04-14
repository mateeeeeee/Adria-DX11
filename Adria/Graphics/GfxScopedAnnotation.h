#pragma once
#include <d3d11_1.h>


namespace adria
{
	class GfxScopedAnnotation
	{
	public:
		GfxScopedAnnotation(ID3DUserDefinedAnnotation* annotation, LPCWSTR name)
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

	#define SCOPED_ANNOTATION(annotation, name) GfxScopedAnnotation _annotation(annotation, name)
}