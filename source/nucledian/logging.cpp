#include "logging.h"

#include<iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <tuple>

namespace nc {
	namespace logging {

		static LoggingFunction PLAIN_STDOUT_WRITER = [](const std::string& message, const LoggingContext& ctx) {
			(void)ctx;
			std::cout << message << std::endl;
		};
		static LoggingFunction IMPORTANT_STDERR_WRITER = [](const std::string& message, const LoggingContext& ctx) {
			const char* const heading = ctx.severity == LoggingSeverity::Error ? "!ERROR" : "Warning";
			std::cerr << heading << ": \"" << message << "\" in " << ctx.file_name<< ", ln." << ctx.line_number << std::endl;
		};



		class Logger {

		  public:
			Logger(std::initializer_list<std::pair<LoggingSeverity, const LoggingFunction&>> to_register) {
				for (auto& kv : to_register) {
					this->register_logging_output(kv.first, kv.second);
				}
			}

			void log_message(const LoggingSeverity severity, const std::string& message, const LoggingContext& ctx) {
				auto bucket = registered_logging_functions.find(severity);
				if (bucket == registered_logging_functions.end())
					return;
				LoggingContext context(ctx);
				context.severity = severity;

				for (auto& f : bucket->second) {
					(*f)(message, context);
				}
			}

			void register_logging_output(LoggingSeverity severity, const LoggingFunction& output)
			{
				registered_logging_functions[severity].push_back(&output);
			}

			void unregister_logging_output(LoggingSeverity severity, const LoggingFunction& output)
			{
				auto& functions = registered_logging_functions[severity];
				functions.erase(std::find(functions.begin(), functions.end(), &output));
			}
		

		private:
			std::unordered_map<LoggingSeverity, std::vector<const LoggingFunction*>> registered_logging_functions;

		};

		static Logger DefaultLogger = Logger{ 
			{ LoggingSeverity::Message, PLAIN_STDOUT_WRITER },
			{ LoggingSeverity::Warning, IMPORTANT_STDERR_WRITER },
			{ LoggingSeverity::Error, IMPORTANT_STDERR_WRITER }
		};




		void nc::logging::log_message_impl(const LoggingSeverity severity, const std::string& message, const LoggingContext &ctx)
		{
			DefaultLogger.log_message(severity, message, ctx);
		}

		void nc::logging::register_logging_output(LoggingSeverity severity, const LoggingFunction& output)
		{
			DefaultLogger.register_logging_output(severity, output);
		}

		void nc::logging::unregister_logging_output(LoggingSeverity severity, const LoggingFunction& output)
		{
			DefaultLogger.unregister_logging_output(severity, output);
		}

	}
}

