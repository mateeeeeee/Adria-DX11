#pragma once
#include <d3d11_1.h>


namespace adria
{
	class ScopedAnnotation
	{
	public:
		ScopedAnnotation(ID3DUserDefinedAnnotation* annotation, LPCWSTR name)
			: annotation(annotation)
		{
			if(annotation) annotation->BeginEvent(name);
		}

		~ScopedAnnotation()
		{
			if (annotation) annotation->EndEvent();
		}

	private:
		ID3DUserDefinedAnnotation* annotation;
	};

#define DECLARE_SCOPED_ANNOTATION(annotation, name) ScopedAnnotation _annotation(annotation, name)
}