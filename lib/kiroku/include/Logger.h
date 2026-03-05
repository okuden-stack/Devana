/**
 * @file    Logger.h
 * @brief   Main logging API — thread-safe, multi-sink logger
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 *
 * Usage:
 *   auto logger = devana::logging::Logger::create(LogConfig::getDevConfig());
 *   Logger::setGlobal(logger);
 *   LOG_INFO("Devana started");
 *   LOG_DEBUG("Classified {} threats", count);
 */

#pragma once

#include <memory>
#include <mutex>
#include <sstream>
#include <vector>

#include "ILogSink.h"
#include "LogConfig.h"
#include "LogFormatter.h"
#include "LogLevel.h"

namespace devana::logging
{

    class Logger
    {
    public:
        static std::shared_ptr<Logger> create(const LogConfig &config = LogConfig::getDefault());

        static std::shared_ptr<Logger> &global();
        static void setGlobal(std::shared_ptr<Logger> logger);

        void addSink(LogSinkPtr sink);
        void clearSinks();

        void setComponent(const std::string &component) { m_component = component; }
        const std::string &getComponent() const { return m_component; }

        void setFormat(const std::string &format) { m_formatter.setFormat(format); }

        // ── Plain string overloads ────────────────────────────────────────────
        void debug(const std::string &message)    { log(LogLevel::DEBUG,    message); }
        void info(const std::string &message)     { log(LogLevel::INFO,     message); }
        void warning(const std::string &message)  { log(LogLevel::WARNING,  message); }
        void error(const std::string &message)    { log(LogLevel::ERROR,    message); }
        void critical(const std::string &message) { log(LogLevel::CRITICAL, message); }

        // ── Variadic template overloads (use {} as placeholders) ─────────────
        template <typename... Args>
        void debug(const std::string &fmt, Args &&...args)
            { log(LogLevel::DEBUG,    formatMessage(fmt, std::forward<Args>(args)...)); }

        template <typename... Args>
        void info(const std::string &fmt, Args &&...args)
            { log(LogLevel::INFO,     formatMessage(fmt, std::forward<Args>(args)...)); }

        template <typename... Args>
        void warning(const std::string &fmt, Args &&...args)
            { log(LogLevel::WARNING,  formatMessage(fmt, std::forward<Args>(args)...)); }

        template <typename... Args>
        void error(const std::string &fmt, Args &&...args)
            { log(LogLevel::ERROR,    formatMessage(fmt, std::forward<Args>(args)...)); }

        template <typename... Args>
        void critical(const std::string &fmt, Args &&...args)
            { log(LogLevel::CRITICAL, formatMessage(fmt, std::forward<Args>(args)...)); }

        void flush();

    private:
        Logger() = default;

        void log(LogLevel level, const std::string &message);

        template <typename... Args>
        std::string formatMessage(const std::string &format, Args &&...args)
        {
            std::ostringstream oss;
            formatMessageImpl(oss, format, std::forward<Args>(args)...);
            return oss.str();
        }

        template <typename T, typename... Args>
        void formatMessageImpl(std::ostringstream &oss, const std::string &format,
                               T &&value, Args &&...args)
        {
            size_t pos = format.find("{}");
            if (pos != std::string::npos)
            {
                oss << format.substr(0, pos) << std::forward<T>(value);
                formatMessageImpl(oss, format.substr(pos + 2), std::forward<Args>(args)...);
            }
            else
            {
                oss << format;
            }
        }

        void formatMessageImpl(std::ostringstream &oss, const std::string &format)
        {
            oss << format;
        }

        std::vector<LogSinkPtr> m_sinks;
        LogFormatter m_formatter;
        std::string m_component;
        mutable std::mutex m_mutex;

        static std::shared_ptr<Logger> s_globalLogger;
    };

} // namespace devana::logging

// ── Convenience macros ────────────────────────────────────────────────────────
#define LOG_DEBUG(...)    devana::logging::Logger::global()->debug(__VA_ARGS__)
#define LOG_INFO(...)     devana::logging::Logger::global()->info(__VA_ARGS__)
#define LOG_WARNING(...)  devana::logging::Logger::global()->warning(__VA_ARGS__)
#define LOG_ERROR(...)    devana::logging::Logger::global()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) devana::logging::Logger::global()->critical(__VA_ARGS__)
