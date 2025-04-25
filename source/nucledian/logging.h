#pragma once

#include <string>
#include <format>
#include <functional>

#include <metaprogramming.h>

namespace nc {
	namespace logging {

		enum class LoggingSeverity {
			Unset, Message, Warning, Error
		};

		struct LoggingContext {
			LoggingSeverity severity;
			const char* file_name;
			const char* line_number;
		};

#define CAPTURE_CURRENT_LOGGING_CONTEXT() ( nc::logging::LoggingContext{.severity = nc::logging::LoggingSeverity::Unset, .file_name = STRINGIFY(__FILE__), .line_number = STRINGIFY(__LINE__) })

		using LoggingFunction = std::function<void(const std::string &mesage, const LoggingContext &ctx)>;


		void log_message_impl(const LoggingSeverity severity, const std::string& message, const LoggingContext& ctx);


		void register_logging_output(LoggingSeverity severity, const LoggingFunction &output);
		void unregister_logging_output(LoggingSeverity severity, const LoggingFunction &output);



#		define NC_LOG_GENERIC(severity, ...) nc::logging::log_message_impl((severity), std::format("" __VA_ARGS__), CAPTURE_CURRENT_LOGGING_CONTEXT())
#		define NC_MESSAGE(...)		NC_LOG_GENERIC(nc::logging::LoggingSeverity::Message, __VA_ARGS__)
#		define NC_WARNING(...)		NC_LOG_GENERIC(nc::logging::LoggingSeverity::Warning, __VA_ARGS__)
#		define NC_LOG_ERROR(...)	NC_LOG_GENERIC(nc::logging::LoggingSeverity::Error, __VA_ARGS__)


	}
}