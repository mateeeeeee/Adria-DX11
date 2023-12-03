#pragma once
#include "Core/Logger.h"

namespace adria
{
	struct ImGuiLogger;
	class EditorLogger : public ILogger
	{
	public:
		explicit EditorLogger(LogLevel logger_level = LogLevel::LOG_DEBUG);
		virtual void Log(LogLevel level, char const* entry, char const* file, uint32_t line) override;
		void Draw(const char* title, bool* p_open);

	private:
		std::unique_ptr<ImGuiLogger> logger;
		LogLevel logger_level;
	};
}