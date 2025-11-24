#include "logger.hpp"

void initLogger()
{
    spdlog::init_thread_pool(8192,1);

    auto console_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    auto file_sink    = std::make_shared<spdlog::sinks::rotating_file_sink_mt>
    (
        "log/system.log",  // file address
        1024 * 1024 * 2,   // 2 MB per file
        5                  // 5 files

    );

    auto logger = std::make_shared<spdlog::async_logger>
    (
        "main",
        spdlog::sinks_init_list{console_sink,file_sink},
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block
    );
    //enable logger
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
    //set pattern 
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [T%t] %v");
    spdlog::set_level(spdlog::level::debug);     // enable all log levels
}