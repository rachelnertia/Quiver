#pragma once

#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace qvr
{

void InitLoggers(const spdlog::level::level_enum all);

inline std::shared_ptr<spdlog::logger> GetConsoleLogger()
{
	auto log = spdlog::get("console");
	assert(log);
	return log;
}

}