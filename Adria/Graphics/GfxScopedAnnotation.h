#pragma once

namespace adria
{
	class GfxCommandContext;
	class GfxScopedAnnotation
	{
	public:
		GfxScopedAnnotation(GfxCommandContext* context, char const* name);

		~GfxScopedAnnotation();

	private:
		GfxCommandContext* context;
	};
	#define AdriaGfxScopedAnnotation(ctx, name) GfxScopedAnnotation ADRIA_CONCAT(_annotation, __COUNTER__)(ctx, name)
}