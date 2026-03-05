/**
 * @file    main.cpp
 * @brief   Sentinel application entry point
 * @author  Sentinel Team
 * @date    2026-02-03
 */

#include <QApplication>
#include "ui/MainWindow/MainWindow.h"
#include <Logger.h>

using namespace sentinel::logging;

int main(int argc, char *argv[])
{
    // Initialize Qt Application
    QApplication app(argc, argv);
    app.setApplicationName("Sentinel");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Sentinel");

    // Initialize logging system with development config (console + colors)
    LogConfig logConfig = LogConfig::getDevConfig();
    logConfig.component = "Sentinel";
    logConfig.level = LogLevel::DEBUG;  // Show all messages during development

    auto logger = Logger::create(logConfig);
    Logger::setGlobal(logger);

    LOG_INFO("=======================================================");
    LOG_INFO("  Sentinel Intelligence Platform v1.0.0");
    LOG_INFO("=======================================================");
    LOG_INFO("Starting application...");
    LOG_DEBUG("Qt version: {}", qVersion());

    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();

    LOG_INFO("Application started successfully");

    // Run event loop
    int result = app.exec();

    LOG_INFO("Application shutting down with exit code: {}", result);
    logger->flush();

    return result;
}
