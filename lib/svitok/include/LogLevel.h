/**
 * @file    LogLevel.h
 * @brief   Log level enumeration and utilities
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 */

#pragma once

#include <string>
#include <string_view>

namespace devana::logging
{

    enum class LogLevel
    {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3,
        CRITICAL = 4
    };

    constexpr std::string_view toString(LogLevel level) noexcept
    {
        switch (level)
        {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
        }
    }

    constexpr std::string_view toShortString(LogLevel level) noexcept
    {
        switch (level)
        {
        case LogLevel::DEBUG:    return "DBG";
        case LogLevel::INFO:     return "INF";
        case LogLevel::WARNING:  return "WRN";
        case LogLevel::ERROR:    return "ERR";
        case LogLevel::CRITICAL: return "CRT";
        default:                 return "???";
        }
    }

    inline LogLevel fromString(std::string_view str) noexcept
    {
        std::string upper;
        upper.reserve(str.size());
        for (char c : str)
            upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        if (upper == "DEBUG"    || upper == "DBG")               return LogLevel::DEBUG;
        if (upper == "INFO"     || upper == "INF")               return LogLevel::INFO;
        if (upper == "WARNING"  || upper == "WARN" || upper == "WRN") return LogLevel::WARNING;
        if (upper == "ERROR"    || upper == "ERR")               return LogLevel::ERROR;
        if (upper == "CRITICAL" || upper == "CRIT" || upper == "CRT") return LogLevel::CRITICAL;

        return LogLevel::INFO;
    }

    constexpr std::string_view getColorCode(LogLevel level) noexcept
    {
        switch (level)
        {
        case LogLevel::DEBUG:    return "\033[36m";   // Cyan
        case LogLevel::INFO:     return "\033[32m";   // Green
        case LogLevel::WARNING:  return "\033[33m";   // Yellow
        case LogLevel::ERROR:    return "\033[31m";   // Red
        case LogLevel::CRITICAL: return "\033[1;31m"; // Bold Red
        default:                 return "\033[0m";
        }
    }

    constexpr std::string_view COLOR_RESET = "\033[0m";

    constexpr bool isEnabled(LogLevel level, LogLevel minLevel) noexcept
    {
        return static_cast<int>(level) >= static_cast<int>(minLevel);
    }

} // namespace devana::logging
