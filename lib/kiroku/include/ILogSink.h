/**
 * @file    ILogSink.h
 * @brief   Interface for log output destinations
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 */

#pragma once

#include <memory>
#include <string>

#include "LogLevel.h"

namespace devana::logging
{

    /**
     * @class ILogSink
     * @brief Abstract interface for log output destinations
     *
     * Implementations must be thread-safe — multiple threads may call
     * write() concurrently.
     */
    class ILogSink
    {
    public:
        virtual ~ILogSink() = default;

        virtual void write(LogLevel level, const std::string &timestamp,
                           const std::string &component, const std::string &message) = 0;

        virtual void flush() = 0;
        virtual void setMinLevel(LogLevel level) = 0;
        virtual LogLevel getMinLevel() const = 0;
        virtual bool shouldLog(LogLevel level) const { return isEnabled(level, getMinLevel()); }
        virtual std::string getName() const = 0;
    };

    using LogSinkPtr = std::shared_ptr<ILogSink>;

    /**
     * @class NullSink
     * @brief Discards all messages (useful for testing/disabling output)
     */
    class NullSink : public ILogSink
    {
    public:
        void write(LogLevel, const std::string &, const std::string &, const std::string &) override {}
        void flush() override {}
        void setMinLevel(LogLevel level) override { m_minLevel = level; }
        LogLevel getMinLevel() const override { return m_minLevel; }
        std::string getName() const override { return "NullSink"; }

    private:
        LogLevel m_minLevel = LogLevel::DEBUG;
    };

} // namespace devana::logging
