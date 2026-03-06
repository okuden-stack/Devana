/**
 * @file    main.cpp
 * @brief   Devana — headless threat classification subsystem
 * @author  AK
 *
 * In production this will be a QCoreApplication with no window.
 * The test harness (UnitTestInjector / UnitTestExtractor) requires ZeroMQ
 * and is compiled only when DEVANA_ENABLE_ZMQ is set.
 *
 * ── ENABLING THE TEST HARNESS ────────────────────────────────────────────────
 *
 *   Install ZeroMQ first:
 *     macOS:         brew install zeromq cppzmq
 *     Debian/Ubuntu: sudo apt install libzmq3-dev
 *
 *   Then rebuild with the option enabled:
 *     ./devana rebuild  (after setting zmq_harness: true in product.json)
 *   or manually:
 *     cmake -DDEVANA_ENABLE_ZMQ=ON ..
 *
 * ── CLI USAGE (when harness is enabled) ──────────────────────────────────────
 *
 *   -s, --scenario <name>        Preselect a scenario by name on startup
 *   -f, --scenarios-file <path>  Load a custom test_scenarios.json
 *   -l, --list-scenarios         Print scenario names and exit
 *   -h, --help                   Show help and exit
 *   -v, --version                Show version and exit
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QLabel>
#include <Logger.h>

#ifdef DEVANA_ENABLE_ZMQ
#include <QTextStream>
#include "ui/TestHarness/TestHarnessWindow.h"
#include "ui/TestHarness/ScenarioLoader.h"
#endif

using namespace devana::logging;

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Devana");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Maboroshi Defence Systems");

    // ── Logging ───────────────────────────────────────────────────────────
    LogConfig logConfig = LogConfig::getDevConfig();
    logConfig.component = "Devana";
    logConfig.level     = LogLevel::DEBUG;

    auto logger = Logger::create(logConfig);
    Logger::setGlobal(logger);

    LOG_INFO("=======================================================");
    LOG_INFO("  Devana Threat Intelligence Subsystem v0.1.0");
    LOG_INFO("=======================================================");

#ifdef DEVANA_ENABLE_ZMQ

    // ── CLI argument parser ───────────────────────────────────────────────
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Devana Threat Intelligence Subsystem — Test Harness");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption scenarioOpt(
        {"s", "scenario"},
        "Preselect a scenario by name on startup.",
        "name");
    parser.addOption(scenarioOpt);

    QCommandLineOption fileOpt(
        {"f", "scenarios-file"},
        "Load scenarios from this JSON file (default: tools/config/test_scenarios.json).",
        "path");
    parser.addOption(fileOpt);

    QCommandLineOption listOpt(
        {"l", "list-scenarios"},
        "Print all available scenario names and exit.");
    parser.addOption(listOpt);

    parser.process(app);

    // ── Resolve scenarios file ────────────────────────────────────────────
    QString scenariosFile = parser.isSet(fileOpt)
                                ? parser.value(fileOpt)
                                : ScenarioLoader::findDefaultPath();

    if (scenariosFile.isEmpty())
    {
        LOG_WARNING("Could not locate test_scenarios.json — "
                    "use --scenarios-file to specify one");
    }

    // ── --list-scenarios: print and exit ──────────────────────────────────
    if (parser.isSet(listOpt))
    {
        QTextStream out(stdout);

        if (scenariosFile.isEmpty())
        {
            out << "No scenarios file found.\n"
                   "Use --scenarios-file <path> to specify one.\n";
            return 1;
        }

        ScenarioLoader loader(scenariosFile);
        if (!loader.isValid())
        {
            out << "Failed to load " << scenariosFile << ":\n"
                << loader.lastError() << "\n";
            return 1;
        }

        out << "Scenarios in " << scenariosFile << ":\n\n";
        const QStringList names = loader.scenarioNames();
        for (int i = 0; i < names.size(); ++i)
            out << QString("  %1. %2\n").arg(i + 1).arg(names[i]);
        out << "\n";
        return 0;
    }

    // ── Open test harness window ──────────────────────────────────────────
    QString initialScenario = parser.isSet(scenarioOpt)
                                  ? parser.value(scenarioOpt)
                                  : QString{};

    LOG_INFO("  [TEST HARNESS MODE]");
    LOG_DEBUG("scenarios-file:    {}",
              scenariosFile.isEmpty() ? "(not found)" : scenariosFile.toStdString());
    LOG_DEBUG("initial-scenario:  {}",
              initialScenario.isEmpty() ? "(first)" : initialScenario.toStdString());

    TestHarnessWindow window(initialScenario, scenariosFile);
    window.show();

#else

    // ── Harness disabled — placeholder window ─────────────────────────────
    LOG_INFO("  [STUB MODE — test harness not built]");
    LOG_INFO("  Rebuild with -DDEVANA_ENABLE_ZMQ=ON to enable the test harness.");

    auto* label = new QLabel(
        "<h2 style='font-family:sans-serif; color:#7eb8f7;'>Devana</h2>"
        "<p style='font-family:sans-serif; color:#aaaacc;'>"
        "Test harness not built.<br>"
        "Rebuild with <code>-DDEVANA_ENABLE_ZMQ=ON</code> to enable it.</p>");
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumSize(480, 200);
    label->setStyleSheet("background-color:#1a1a2e;");
    label->setWindowTitle("Devana — Stub");
    label->show();

#endif // DEVANA_ENABLE_ZMQ

    int result = app.exec();

    LOG_INFO("Devana shutting down (exit code: {})", result);
    logger->flush();

    return result;
}
