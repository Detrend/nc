// Project Nuclidean Source File
#pragma once

#include <string>
#include <format>
#include <functional>
#include "types.h"

#include <metaprogramming.h>

namespace nc::logging 
{


enum class LoggingSeverity 
{
    unset, message, warning, error
};

struct LoggingContext 
{
    LoggingSeverity     severity;
    cstr                file_name;
    cstr                line_number;
};

#define CAPTURE_CURRENT_LOGGING_CONTEXT() ( nc::logging::LoggingContext \
{                                                                       \
    .severity         = nc::logging::LoggingSeverity::unset,            \
    .file_name        = STRINGIFY(__FILE__),                            \
    .line_number      = STRINGIFY(__LINE__)                             \
})


using LoggingFunction = std::function<void(const std::string &mesage, const LoggingContext &ctx)>;


void    log_message_impl                (const LoggingSeverity severity, const std::string& message, const LoggingContext& ctx);

void    register_logging_output         (LoggingSeverity severity, const LoggingFunction &output);
void    unregister_logging_output       (LoggingSeverity severity, const LoggingFunction &output);


// Throw this exception on error in constexpr functions, either causing compiler error or logging to nc_crit() at runtime
struct crit_exception_t {
  constexpr crit_exception_t([[maybe_unused]] const std::string& message, [[maybe_unused]] const nc::logging::LoggingContext& logging_context)
  {
    if (! std::is_constant_evaluated()) {
      nc::logging::log_message_impl(nc::logging::LoggingSeverity::error, message, logging_context);
    }
  }
};

#define NC_LOG_GENERIC(severity, ...)   nc::logging::log_message_impl((severity), std::format("" __VA_ARGS__), CAPTURE_CURRENT_LOGGING_CONTEXT())

#define nc_log(...)  NC_LOG_GENERIC(nc::logging::LoggingSeverity::message, __VA_ARGS__)
#define nc_warn(...) NC_LOG_GENERIC(nc::logging::LoggingSeverity::warning, __VA_ARGS__)
#define nc_crit(...) NC_LOG_GENERIC(nc::logging::LoggingSeverity::error, __VA_ARGS__)
#define nc_crit_constexpr(...) throw ::nc::logging::crit_exception_t(std::format("" __VA_ARGS__), CAPTURE_CURRENT_LOGGING_CONTEXT())

}
