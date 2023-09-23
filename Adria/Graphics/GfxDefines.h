#pragma once
#include "Core/Defines.h"

#define GFX_CHECK_HR(hr) if(FAILED(hr)) ADRIA_DEBUGBREAK();
#define GFX_BACKBUFFER_COUNT 3