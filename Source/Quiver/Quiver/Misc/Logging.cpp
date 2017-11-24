#include "Logging.h"

#include <spdlog/spdlog.h>

namespace qvr
{

void InitLoggers(const spdlog::level::level_enum globalLevel)
{
	auto consoleLog = spdlog::get("console");
	
	if (consoleLog == nullptr) {
		// Create console logger with colour.
		consoleLog = spdlog::stdout_color_mt("console");
		
		consoleLog->set_pattern("[%l] %v"); // [level] msg
	}

	consoleLog->set_level(globalLevel);
}

}