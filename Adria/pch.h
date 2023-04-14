#pragma once
//std headers + win32/d3d12
#include <vector>
#include <memory>
#include <string>
#include <array>
#include <queue>
#include <mutex>
#include <thread>
#include <optional>
#include <functional>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <d3d11.h>
#include <wrl.h>
#include <dxgi1_3.h>
#include <windows.h>

//external utility
#include <DirectXMath.h>
#include "nfd.h"
#include "cereal/archives/binary.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "ImGui/imgui.h"
#include "ImGui/ImGuizmo.h"

//
#include "Core/CoreTypes.h"
#include "Core/Defines.h"
