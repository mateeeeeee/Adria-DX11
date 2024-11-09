#pragma once
#include "Core/Macros.h"

#define GFX_CHECK_HR(hr) if(FAILED(hr)) ADRIA_DEBUGBREAK();
#define GFX_BACKBUFFER_COUNT 3
#define GFX_PROFILING 1