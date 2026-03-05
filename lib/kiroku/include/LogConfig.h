/**
 * @file    LogConfig.h
 * @brief   Logging configuration structures and presets
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 */

#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "FileSink.h"
#include "LogLevel.h"

namespace devana::logging
{

    /**
     * @struct LogConfig
     * @brief Full configuration for the logging system
     *
     * Load from JSON: LogConfig::loadFromFile("config/logging.json")
     * or construct programmatically using one of the preset factories.
     */
    struct LogConfig
    {
        LogLevel level = LogLevel::INFO;

        /// "console", "file", "both", or "none"
        std::string output = "both";

        std::filesystem::path filepath = "logs/devana.log";

        RotationConfig rotation;

        std::string format = "{timestamp} [{level}] [{component}] {message}";

        bool useColors = true;

        /// Flush after every write — slower but guarantees no log loss on crash
        bool flushOnWrite = false;

        std::string component = "Devana";

        static std::optional<LogConfig> loadFromFile(const std::filesystem::path &path);
        bool saveToFile(const std::filesystem::path &path) const;

        static LogConfig getDefault() { return LogConfig{}; }

        /// Verbose console output for local development
        static LogConfig getDevConfig()
        {
            LogConfig config;
            config.level = LogLevel::DEBUG;
            config.output = "console";
            config.useColors = true;
            config.format = "{timestamp} [{level_short}] {message}";
            return config;
        }

        /// File-only, rotating, flush-on-write for production deployments
        static LogConfig getProdConfig()
        {
            LogConfig config;
            config.level = LogLevel::INFO;
            config.output = "file";
            config.filepath = "/var/log/devana/devana.log";
            config.rotation.enabled = true;
            config.rotation.maxSizeBytes = 50 * 1024 * 1024; // 50 MB
            config.rotation.maxFiles = 10;
            config.useColors = false;
            config.flushOnWrite = true;
            return config;
        }
    };

} // namespace devana::logging
