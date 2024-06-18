#pragma once
#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>

class Logger {
public:
    enum Level {
        INFO,
        WARNING,
        ERROR
    };

    static void log(const std::string& message, Level level = INFO);

private:
    static void logInfo(const std::string& message);
    static void logWarning(const std::string& message);
    static void logError(const std::string& message);
};

#endif // LOGGER_H
