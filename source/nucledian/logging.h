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



#define NC_LOG_GENERIC(severity, ...)   nc::logging::log_message_impl((severity), std::format("" __VA_ARGS__), CAPTURE_CURRENT_LOGGING_CONTEXT())

#define nc_log(...)		                NC_LOG_GENERIC(nc::logging::LoggingSeverity::message, __VA_ARGS__)
#define nc_warn(...)	                NC_LOG_GENERIC(nc::logging::LoggingSeverity::warning, __VA_ARGS__)
#define nc_crit(...)	                NC_LOG_GENERIC(nc::logging::LoggingSeverity::error, __VA_ARGS__)





}