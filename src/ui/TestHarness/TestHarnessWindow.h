/**
 * @file    TestHarnessWindow.h
 * @brief   Top-level window — houses the injector and extractor side-by-side
 * @author  AK
 *
 * Scenarios are loaded from test_scenarios.json via ScenarioLoader.
 * Pass an initial scenario name (from --scenario CLI arg) to the constructor
 * to pre-select it on startup. Pass a custom file path to override the
 * default search.
 */

#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QComboBox>
#include <QLabel>
#include <QToolBar>

#include "ScenarioLoader.h"
#include "UnitTestInjector/UnitTestInjector.h"
#include "UnitTestExtractor/UnitTestExtractor.h"
#include "TestField.h"

class TestHarnessWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @param initialScenario  Scenario name to select on startup.
     *                         Empty string → first scenario in file.
     * @param scenariosFile    Path to test_scenarios.json.
     *                         Empty string → ScenarioLoader::findDefaultPath().
     * @param parent           Parent widget.
     */
    explicit TestHarnessWindow(const QString& initialScenario = {},
                               const QString& scenariosFile   = {},
                               QWidget*       parent          = nullptr);
    ~TestHarnessWindow() override = default;

    UnitTestInjector*  injector()  { return m_injector;  }
    UnitTestExtractor* extractor() { return m_extractor; }

private slots:
    void onScenarioChanged(int index);
    void onInjected(const QString& topic, const QString& payload);
    void onMessageReceived(const QString& topic, const QVariantMap& fields);

private:
    void buildUI();
    void applyStyles();
    void populatePicker();
    void applyScenario(const Scenario& s);

    ScenarioLoader     m_loader;
    UnitTestInjector*  m_injector       = nullptr;
    UnitTestExtractor* m_extractor      = nullptr;
    QSplitter*         m_splitter       = nullptr;
    QComboBox*         m_scenarioPicker = nullptr;
    QLabel*            m_statusLabel    = nullptr;
    QString            m_initialScenario;
};
