/**
 * @file    LogFormatter.h
 * @brief   Log message formatting utilities
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 *
 * Supported placeholders:
 *   {timestamp}   — ISO 8601 (2026-03-03T14:30:45.123Z)
 *   {level}       — DEBUG / INFO / WARNING / ERROR / CRITICAL
 *   {level_short} — DBG / INF / WRN / ERR / CRT
 *   {component}   — component tag
 *   {message}     — message body
 *   {thread}      — thread ID
 */

#pragma once

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>

#include "LogLevel.h"

namespace devana::logging
{

    class LogFormatter
    {
    public:
        explicit LogFormatter(const std::string &format = getDefaultFormat()) : m_format(format) {}

        std::string format(LogLevel level, const std::string &component,
                           const std::string &message,
                           const char *file = nullptr, int line = 0) const;

        static std::string getCurrentTimestamp();

        static std::string getDefaultFormat() { return "{timestamp} [{level}] [{component}] {message}"; }
        static std::string getCompactFormat()  { return "[{level_short}] {message}"; }
        static std::string getJsonFormat()
        {
            return R"({"time":"{timestamp}","level":"{level}","component":"{component}","msg":"{message}"})";
        }

        void setFormat(const std::string &format) { m_format = format; }
        const std::string &getFormat() const { return m_format; }

    private:
        static void replacePlaceholder(std::string &str, const std::string &placeholder,
                                       const std::string &value);
        static std::string getThreadId();

        std::string m_format;
    };

} // namespace devana::logging
