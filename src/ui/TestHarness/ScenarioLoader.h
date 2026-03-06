/**
 * @file    ScenarioLoader.h
 * @brief   Loads test scenarios from test_scenarios.json into TestSchema objects
 * @author  AK
 *
 * Parses tools/config/test_scenarios.json (or any path you supply) and
 * produces ready-to-use InjectorScenario / ExtractorScenario structs that
 * can be handed directly to UnitTestInjector::setSchema() and
 * UnitTestExtractor::setSchema().
 *
 * Usage:
 *   ScenarioLoader loader("tools/config/test_scenarios.json");
 *   if (!loader.isValid()) { ... }
 *
 *   QStringList names = loader.scenarioNames();
 *   auto scenario = loader.scenario("Detection Event (Sentinel → Devana)");
 *   injector->setSchema(scenario.injector.topic, scenario.injector.fields);
 *   extractor->setSchema(scenario.extractor.topic, scenario.extractor.fields);
 */

#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <optional>

#include "TestField.h"

// ── Per-side (injector or extractor) data ────────────────────────────────────

struct SideSchema
{
    QString    topic;
    QString    endpoint;
    TestSchema fields;

    bool isNull() const { return topic.isEmpty() && fields.isEmpty(); }
};

// ── One complete scenario ─────────────────────────────────────────────────────

struct Scenario
{
    QString    name;
    SideSchema injector;
    SideSchema extractor;
};

// ── Loader ────────────────────────────────────────────────────────────────────

class ScenarioLoader
{
public:
    /**
     * @brief Construct and immediately parse the given file.
     * @param path Absolute or relative path to a test_scenarios.json file.
     */
    explicit ScenarioLoader(const QString& path = "");

    /** @brief Re-parse a (possibly different) file. Returns true on success. */
    bool load(const QString& path);

    /** @brief True if the last load() call succeeded and produced ≥1 scenario. */
    bool isValid() const { return m_valid; }

    /** @brief Human-readable error from the last failed load(). */
    QString lastError() const { return m_lastError; }

    /** @brief Ordered list of scenario names (for populating a combo box). */
    QStringList scenarioNames() const;

    /** @brief Look up a scenario by name. Returns std::nullopt if not found. */
    std::optional<Scenario> scenario(const QString& name) const;

    /** @brief First scenario, or std::nullopt if none loaded. */
    std::optional<Scenario> first() const;

    /** @brief All loaded scenarios. */
    const QList<Scenario>& scenarios() const { return m_scenarios; }

    /**
     * @brief Try to locate test_scenarios.json relative to the executable.
     *
     * Search order:
     *   1. <exe_dir>/tools/config/test_scenarios.json
     *   2. <exe_dir>/../tools/config/test_scenarios.json   (build-tree layout)
     *   3. <exe_dir>/../../tools/config/test_scenarios.json
     *   4. <cwd>/tools/config/test_scenarios.json
     *
     * Returns an empty string if not found.
     */
    static QString findDefaultPath();

private:
    static TestSchema parseFields(const QJsonArray& arr);
    static SideSchema parseSide(const QJsonObject& obj,
                                const QString& defaultEndpointBind,
                                const QString& defaultEndpointConnect,
                                bool isBind);

    QList<Scenario> m_scenarios;
    bool            m_valid     = false;
    QString         m_lastError;
};
