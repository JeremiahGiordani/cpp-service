#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace sar_atr {

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

/**
 * @class Logger
 * @brief Simple logger for service output
 */
class Logger {
public:
    static void log(LogLevel level, const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
        
        switch(level) {
            case LogLevel::INFO:
                ss << "[INFO]    ";
                break;
            case LogLevel::WARNING:
                ss << "[WARNING] ";
                break;
            case LogLevel::ERROR:
                ss << "[ERROR]   ";
                break;
            case LogLevel::DEBUG:
                ss << "[DEBUG]   ";
                break;
        }
        
        ss << message;
        std::cout << ss.str() << std::endl;
    }
    
    static void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }
    
    static void warning(const std::string& message) {
        log(LogLevel::WARNING, message);
    }
    
    static void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }
    
    static void debug(const std::string& message) {
        log(LogLevel::DEBUG, message);
    }
};

} // namespace sar_atr

#endif // LOGGER_H
