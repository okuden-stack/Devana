/**
 * @file    ConsoleSink.h
 * @brief   Console output sink for logging
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 *
 * DEBUG/INFO → stdout, WARNING/ERROR/CRITICAL → stderr.
 * ANSI colors applied per level when writing to a TTY.
 */

#pragma once

#include <iostream>
#include <mutex>

#include "ILogSink.h"

namespace devana::logging
{

    class ConsoleSink : public ILogSink
    {
    public:
        explicit ConsoleSink(bool useColors = isTerminal(), LogLevel minLevel = LogLevel::INFO);

        void write(LogLevel level, const std::string &timestamp,
                   const std::string &component, const std::string &message) override;

        void flush() override;
        void setMinLevel(LogLevel level) override { m_minLevel = level; }
        LogLevel getMinLevel() const override { return m_minLevel; }
        std::string getName() const override { return "ConsoleSink"; }

        void setColorsEnabled(bool enable) { m_useColors = enable; }
        bool areColorsEnabled() const { return m_useColors; }

    private:
        static bool isTerminal();
        static std::ostream &getStream(LogLevel level);

        LogLevel m_minLevel;
        bool m_useColors;
        mutable std::mutex m_mutex;
    };

} // namespace devana::logging
