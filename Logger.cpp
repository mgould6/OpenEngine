#include "Logger.h"

void Logger::log(const std::string& message, Level level) {
    switch (level) {
    case INFO:
        logInfo(message);
        break;
    case WARNING:
        logWarning(message);
        break;
    case ERROR:
        logError(message);
        break;
    }
}

void Logger::logInfo(const std::string& message) {
    std::cout << "INFO: " << message << std::endl;
}

void Logger::logWarning(const std::string& message) {
    std::cerr << "WARNING: " << message << std::endl;
}

void Logger::logError(const std::string& message) {
    std::cerr << "ERROR: " << message << std::endl;
}
