#pragma once
#include <string>
#include "../Core/Definitions.h"
#include <shellapi.h>

namespace adria
{

	struct command_line_config_info_t
	{
		u32 window_width = 1080;
		u32 window_height = 720;
		std::string window_title = "adria";
		bool window_maximize = false;
		bool vsync = true;
		std::string log_file = "adria.log";
	};

	command_line_config_info_t ParseCommandLine(LPWSTR command_line);
}