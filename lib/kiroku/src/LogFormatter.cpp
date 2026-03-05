/**
 * @file    LogFormatter.cpp
 * @brief   Log message formatting implementation
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 */

#include "LogFormatter.h"

#include <iomanip>
#include <sstream>
#include <thread>

namespace devana::logging
{

    std::string LogFormatter::format(LogLevel level, const std::string &component,
                                     const std::string &message,
                                     const char *file, int line) const
    {
        std::string result = m_format;

        replacePlaceholder(result, "{timestamp}",   getCurrentTimestamp());
        replacePlaceholder(result, "{level}",       std::string(toString(level)));
        replacePlaceholder(result, "{level_short}", std::string(toShortString(level)));
        replacePlaceholder(result, "{component}",   component);
        replacePlaceholder(result, "{message}",     message);
        replacePlaceholder(result, "{thread}",      getThreadId());

        if (file != nullptr)
            replacePlaceholder(result, "{file}", file);

        if (line > 0)
            replacePlaceholder(result, "{line}", std::to_string(line));

        return result;
    }

    std::string LogFormatter::getCurrentTimestamp()
    {
        using namespace std::chrono;

        auto now = system_clock::now();
        auto ms  = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        auto t   = system_clock::to_time_t(now);

        std::tm tm;
        gmtime_r(&t, &tm);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';

        return oss.str();
    }

    void LogFormatter::replacePlaceholder(std::string &str, const std::string &placeholder,
                                          const std::string &value)
    {
        size_t pos = 0;
        while ((pos = str.find(placeholder, pos)) != std::string::npos)
        {
            str.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }

    std::string LogFormatter::getThreadId()
    {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        return oss.str();
    }

} // namespace devana::logging
