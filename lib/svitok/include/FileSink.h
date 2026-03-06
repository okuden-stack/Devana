/**
 * @file    FileSink.h
 * @brief   File output sink with automatic rotation
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 */

#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>

#include "ILogSink.h"

namespace devana::logging
{

    /**
     * @struct RotationConfig
     * @brief Configuration for log file rotation
     */
    struct RotationConfig
    {
        bool enabled = true;
        size_t maxSizeBytes = 10 * 1024 * 1024; ///< 10 MB default
        size_t maxFiles = 5;
        bool compressOld = false;
    };

    /**
     * @class FileSink
     * @brief Log sink that writes to file with automatic size-based rotation
     *
     * When the file reaches maxSizeBytes, existing files are shifted:
     * log.txt → log.txt.1 → log.txt.2 … and a fresh log.txt is opened.
     * Files beyond maxFiles are deleted.
     */
    class FileSink : public ILogSink
    {
    public:
        explicit FileSink(const std::filesystem::path &filepath,
                          const RotationConfig &config = RotationConfig{},
                          LogLevel minLevel = LogLevel::DEBUG);

        ~FileSink() override;

        void write(LogLevel level, const std::string &timestamp,
                   const std::string &component, const std::string &message) override;

        void flush() override;
        void setMinLevel(LogLevel level) override { m_minLevel = level; }
        LogLevel getMinLevel() const override { return m_minLevel; }
        std::string getName() const override;

        size_t getCurrentSize() const { return m_currentSize; }
        size_t getRotationCount() const { return m_rotationCount; }

        void rotate();

    private:
        void openFile(bool append = true);
        void closeFile();
        void checkRotation();
        void performRotation();

        std::filesystem::path m_filepath;
        RotationConfig m_config;
        LogLevel m_minLevel;

        std::ofstream m_file;
        std::atomic<size_t> m_currentSize{0};
        std::atomic<size_t> m_rotationCount{0};

        mutable std::mutex m_mutex;
    };

} // namespace devana::logging
