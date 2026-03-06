/**
 * @file    Logger.cpp
 * @brief   Main logging API implementation
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 */

#include "Logger.h"

#include "ConsoleSink.h"
#include "FileSink.h"

namespace devana::logging
{

    std::shared_ptr<Logger> Logger::s_globalLogger = nullptr;

    std::shared_ptr<Logger> Logger::create(const LogConfig &config)
    {
        auto logger = std::shared_ptr<Logger>(new Logger());
        logger->setComponent(config.component);
        logger->setFormat(config.format);

        if (config.output == "console" || config.output == "both")
        {
            logger->addSink(std::make_shared<ConsoleSink>(config.useColors, config.level));
        }

        if (config.output == "file" || config.output == "both")
        {
            logger->addSink(std::make_shared<FileSink>(config.filepath, config.rotation, config.level));
        }

        return logger;
    }

    std::shared_ptr<Logger> &Logger::global()
    {
        if (!s_globalLogger)
            s_globalLogger = create();
        return s_globalLogger;
    }

    void Logger::setGlobal(std::shared_ptr<Logger> logger)
    {
        s_globalLogger = std::move(logger);
    }

    void Logger::addSink(LogSinkPtr sink)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sinks.push_back(std::move(sink));
    }

    void Logger::clearSinks()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sinks.clear();
    }

    void Logger::log(LogLevel level, const std::string &message)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_sinks.empty())
            return;

        std::string timestamp = LogFormatter::getCurrentTimestamp();

        for (auto &sink : m_sinks)
        {
            if (sink && sink->shouldLog(level))
                sink->write(level, timestamp, m_component, message);
        }
    }

    void Logger::flush()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto &sink : m_sinks)
        {
            if (sink)
                sink->flush();
        }
    }

} // namespace devana::logging
