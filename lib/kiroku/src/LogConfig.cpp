/**
 * @file    LogConfig.cpp
 * @brief   Logging configuration implementation
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 */

#include "LogConfig.h"

#include <fstream>
#include <iostream>

namespace devana::logging
{

    std::optional<LogConfig> LogConfig::loadFromFile(const std::filesystem::path &path)
    {
        if (!std::filesystem::exists(path))
        {
            std::cerr << "Config file not found: " << path << std::endl;
            return std::nullopt;
        }

        // TODO: implement full JSON parsing (nlohmann/json or simdjson)
        std::cerr << "JSON parsing not yet implemented. Using default config." << std::endl;
        return getDefault();
    }

    bool LogConfig::saveToFile(const std::filesystem::path &path) const
    {
        std::ofstream file(path);
        if (!file.is_open())
        {
            std::cerr << "Failed to open config file for writing: " << path << std::endl;
            return false;
        }

        file << "# Devana Logging Configuration\n";
        file << "level="                  << toString(level)                         << "\n";
        file << "output="                 << output                                   << "\n";
        file << "filepath="               << filepath.string()                        << "\n";
        file << "rotation.enabled="       << (rotation.enabled ? "true" : "false")   << "\n";
        file << "rotation.maxSizeBytes="  << rotation.maxSizeBytes                   << "\n";
        file << "rotation.maxFiles="      << rotation.maxFiles                       << "\n";
        file << "format="                 << format                                   << "\n";
        file << "useColors="              << (useColors ? "true" : "false")          << "\n";
        file << "flushOnWrite="           << (flushOnWrite ? "true" : "false")       << "\n";

        return true;
    }

} // namespace devana::logging
