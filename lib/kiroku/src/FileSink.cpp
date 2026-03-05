/**
 * @file    FileSink.cpp
 * @brief   File output sink implementation
 * @author  AK
 * @date    2025-11-10
 *
 * This file is part of the Devana project.
 */

#include "FileSink.h"

#include <iostream>

namespace devana::logging
{

    FileSink::FileSink(const std::filesystem::path &filepath,
                       const RotationConfig &config,
                       LogLevel minLevel)
        : m_filepath(filepath), m_config(config), m_minLevel(minLevel)
    {
        auto parent = m_filepath.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent))
            std::filesystem::create_directories(parent);

        openFile(true);

        if (std::filesystem::exists(m_filepath))
            m_currentSize = std::filesystem::file_size(m_filepath);
    }

    FileSink::~FileSink()
    {
        closeFile();
    }

    void FileSink::write(LogLevel level, const std::string &timestamp,
                         const std::string &component, const std::string &message)
    {
        if (!shouldLog(level))
            return;

        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_file.is_open())
            return;

        std::string line = "[" + timestamp + "] [" + std::string(toString(level)) + "] ";
        if (!component.empty())
            line += "[" + component + "] ";
        line += message + "\n";

        m_file << line;
        m_currentSize += line.size();

        checkRotation();
    }

    void FileSink::flush()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open())
            m_file.flush();
    }

    std::string FileSink::getName() const
    {
        return "FileSink(" + m_filepath.string() + ")";
    }

    void FileSink::rotate()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        performRotation();
    }

    void FileSink::openFile(bool append)
    {
        auto mode = append ? (std::ios::out | std::ios::app) : std::ios::out;
        m_file.open(m_filepath, mode);

        if (!m_file.is_open())
            std::cerr << "Failed to open log file: " << m_filepath << std::endl;
    }

    void FileSink::closeFile()
    {
        if (m_file.is_open())
        {
            m_file.flush();
            m_file.close();
        }
    }

    void FileSink::checkRotation()
    {
        if (m_config.enabled && m_currentSize >= m_config.maxSizeBytes)
            performRotation();
    }

    void FileSink::performRotation()
    {
        closeFile();

        std::error_code ec;

        // Shift existing rotated files: .N → .N+1
        for (int i = static_cast<int>(m_config.maxFiles) - 1; i >= 1; --i)
        {
            auto src = m_filepath.string() + "." + std::to_string(i);
            auto dst = m_filepath.string() + "." + std::to_string(i + 1);

            if (std::filesystem::exists(src, ec))
            {
                if (std::filesystem::exists(dst, ec))
                    std::filesystem::remove(dst, ec);
                std::filesystem::rename(src, dst, ec);
            }
        }

        // Current log → .1
        auto rotated = m_filepath.string() + ".1";
        if (std::filesystem::exists(m_filepath, ec))
            std::filesystem::rename(m_filepath, rotated, ec);

        // Delete oldest if over limit
        auto oldest = m_filepath.string() + "." + std::to_string(m_config.maxFiles + 1);
        if (std::filesystem::exists(oldest, ec))
            std::filesystem::remove(oldest, ec);

        m_currentSize = 0;
        ++m_rotationCount;
        openFile(false);
    }

} // namespace devana::logging
