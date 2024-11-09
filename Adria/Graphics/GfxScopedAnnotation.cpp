#include "GfxScopedAnnotation.h"
#include "GfxCommandContext.h"

namespace adria
{


	GfxScopedAnnotation::GfxScopedAnnotation(GfxCommandContext* context, Char const* name) : context(context)
	{
		if (context) context->BeginEvent(name);
	}

	GfxScopedAnnotation::~GfxScopedAnnotation()
	{
		if (context) context->EndEvent();
	}

}

