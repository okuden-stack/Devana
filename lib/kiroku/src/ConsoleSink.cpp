/**
 * @file    ConsoleSink.cpp
 * @brief   Console output sink implementation
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 */

#include "ConsoleSink.h"

#include <unistd.h>

namespace devana::logging
{

    ConsoleSink::ConsoleSink(bool useColors, LogLevel minLevel)
        : m_minLevel(minLevel), m_useColors(useColors) {}

    void ConsoleSink::write(LogLevel level, const std::string &timestamp,
                            const std::string &component, const std::string &message)
    {
        if (!shouldLog(level))
            return;

        std::lock_guard<std::mutex> lock(m_mutex);
        std::ostream &stream = getStream(level);

        if (m_useColors)
            stream << getColorCode(level);

        stream << "[" << timestamp << "] "
               << "[" << toString(level) << "] ";

        if (!component.empty())
            stream << "[" << component << "] ";

        stream << message;

        if (m_useColors)
            stream << COLOR_RESET;

        stream << std::endl;
    }

    void ConsoleSink::flush()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout.flush();
        std::cerr.flush();
    }

    bool ConsoleSink::isTerminal()
    {
        return isatty(STDOUT_FILENO) != 0;
    }

    std::ostream &ConsoleSink::getStream(LogLevel level)
    {
        return (level >= LogLevel::WARNING) ? std::cerr : std::cout;
    }

} // namespace devana::logging
