/**
 * @file    ScenarioLoader.cpp
 * @brief   JSON scenario loader implementation
 * @author  AK
 */

#include "ScenarioLoader.h"
#include <Logger.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace devana::logging;

// ── Constructor ───────────────────────────────────────────────────────────────

ScenarioLoader::ScenarioLoader(const QString& path)
{
    if (!path.isEmpty())
        load(path);
}

// ── Public API ────────────────────────────────────────────────────────────────

bool ScenarioLoader::load(const QString& path)
{
    m_scenarios.clear();
    m_valid     = false;
    m_lastError.clear();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_lastError = QString("Cannot open file: %1").arg(path);
        LOG_WARNING("ScenarioLoader: {}", m_lastError.toStdString());
        return false;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (err.error != QJsonParseError::NoError)
    {
        m_lastError = QString("JSON parse error at offset %1: %2")
                          .arg(err.offset)
                          .arg(err.errorString());
        LOG_WARNING("ScenarioLoader: {}", m_lastError.toStdString());
        return false;
    }

    if (!doc.isObject())
    {
        m_lastError = "Root element must be a JSON object";
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray scenariosArr = root.value("scenarios").toArray();

    if (scenariosArr.isEmpty())
    {
        m_lastError = "No scenarios found in file (missing or empty \"scenarios\" array)";
        LOG_WARNING("ScenarioLoader: {}", m_lastError.toStdString());
        return false;
    }

    for (const QJsonValue& val : scenariosArr)
    {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        Scenario s;
        s.name = obj.value("name").toString().trimmed();
        if (s.name.isEmpty()) continue;

        if (obj.contains("injector"))
            s.injector = parseSide(obj["injector"].toObject(),
                                   "tcp://*:5555", "", /*isBind=*/true);

        if (obj.contains("extractor"))
            s.extractor = parseSide(obj["extractor"].toObject(),
                                    "", "tcp://localhost:5555", /*isBind=*/false);

        m_scenarios.append(s);
        LOG_DEBUG("ScenarioLoader: loaded '{}' — inj fields: {}, ext fields: {}",
                  s.name.toStdString(),
                  s.injector.fields.size(),
                  s.extractor.fields.size());
    }

    if (m_scenarios.isEmpty())
    {
        m_lastError = "File parsed but no valid scenarios found";
        return false;
    }

    m_valid = true;
    LOG_INFO("ScenarioLoader: loaded {} scenario(s) from {}",
             m_scenarios.size(), path.toStdString());
    return true;
}

QStringList ScenarioLoader::scenarioNames() const
{
    QStringList names;
    for (const auto& s : m_scenarios)
        names.append(s.name);
    return names;
}

std::optional<Scenario> ScenarioLoader::scenario(const QString& name) const
{
    for (const auto& s : m_scenarios)
        if (s.name == name)
            return s;
    return std::nullopt;
}

std::optional<Scenario> ScenarioLoader::first() const
{
    if (m_scenarios.isEmpty())
        return std::nullopt;
    return m_scenarios.first();
}

// ── Path discovery ────────────────────────────────────────────────────────────

QString ScenarioLoader::findDefaultPath()
{
    const QString filename = "tools/config/test_scenarios.json";

    // Candidates relative to the executable and cwd
    QStringList candidates;
    if (!QCoreApplication::applicationFilePath().isEmpty())
    {
        QDir exeDir = QFileInfo(QCoreApplication::applicationFilePath()).dir();
        candidates << exeDir.filePath(filename)
                   << exeDir.filePath("../" + filename)
                   << exeDir.filePath("../../" + filename)
                   << exeDir.filePath("../../../" + filename);
    }
    candidates << QDir::current().filePath(filename);

    for (const QString& candidate : candidates)
    {
        if (QFile::exists(candidate))
        {
            LOG_DEBUG("ScenarioLoader: found scenarios at {}",
                      QFileInfo(candidate).absoluteFilePath().toStdString());
            return QFileInfo(candidate).absoluteFilePath();
        }
    }

    return {};
}

// ── Private parsing ───────────────────────────────────────────────────────────

SideSchema ScenarioLoader::parseSide(const QJsonObject& obj,
                                     const QString& defaultBind,
                                     const QString& defaultConnect,
                                     bool isBind)
{
    SideSchema side;
    side.topic    = obj.value("topic").toString();
    side.endpoint = obj.value("endpoint").toString(isBind ? defaultBind : defaultConnect);
    side.fields   = parseFields(obj.value("fields").toArray());
    return side;
}

TestSchema ScenarioLoader::parseFields(const QJsonArray& arr)
{
    TestSchema schema;

    for (const QJsonValue& val : arr)
    {
        if (!val.isObject()) continue;
        QJsonObject f = val.toObject();

        QString name = f.value("name").toString().trimmed();
        QString type = f.value("type").toString().trimmed().toLower();
        if (name.isEmpty() || type.isEmpty()) continue;

        bool    required = f.value("required").toBool(false);
        QString tooltip  = f.value("tooltip").toString();

        if (type == "string")
        {
            schema << TestField::string(name,
                                        f.value("default").toString(),
                                        required, tooltip);
        }
        else if (type == "int")
        {
            schema << TestField::integer(name,
                                         f.value("default").toInt(0),
                                         f.value("min").toInt(0),
                                         f.value("max").toInt(INT_MAX),
                                         required);
        }
        else if (type == "uint64")
        {
            schema << TestField::uint64(name,
                                        static_cast<quint64>(
                                            f.value("default").toDouble(0)),
                                        required);
        }
        else if (type == "real")
        {
            schema << TestField::real(name,
                                      f.value("default").toDouble(0.0),
                                      f.value("min").toDouble(0.0),
                                      f.value("max").toDouble(1e9),
                                      required,
                                      f.value("decimals").toInt(3));
        }
        else if (type == "bool")
        {
            schema << TestField::flag(name,
                                      f.value("default").toBool(false),
                                      required);
        }
        else if (type == "choice")
        {
            QStringList choices;
            for (const auto& c : f.value("choices").toArray())
                choices << c.toString();

            schema << TestField::choice(name, choices,
                                        f.value("default").toString(),
                                        required);
        }
        else if (type == "readonly")
        {
            schema << TestField::readOnly(name, tooltip);
        }
        else
        {
            LOG_WARNING("ScenarioLoader: unknown field type '{}' for field '{}' — skipping",
                        type.toStdString(), name.toStdString());
        }
    }

    return schema;
}
