#pragma once
#include "Core/Logger.h"

namespace adria
{
	struct ImGuiLogger;
	class EditorLogger : public ILogger
	{
	public:
		explicit EditorLogger(LogLevel logger_level = LogLevel::LOG_DEBUG);
		virtual void Log(LogLevel level, Char const* entry, Char const* file, uint32_t line) override;
		void Draw(const Char* title, Bool* p_open);

	private:
		std::unique_ptr<ImGuiLogger> logger;
		LogLevel logger_level;
	};
}