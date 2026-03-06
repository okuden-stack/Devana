/**
 * @file    TestHarnessWindow.cpp
 * @brief   Top-level test harness window — driven by ScenarioLoader
 * @author  AK
 */

#include "TestHarnessWindow.h"
#include <Logger.h>

#include <QApplication>
#include <QDateTime>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QToolBar>
#include <QLabel>
#include <QFileDialog>
#include <QPushButton>

using namespace devana::logging;

TestHarnessWindow::TestHarnessWindow(const QString& initialScenario,
                                     const QString& scenariosFile,
                                     QWidget*       parent)
    : QMainWindow(parent)
    , m_initialScenario(initialScenario)
{
    setWindowTitle("Devana — Test Harness");
    setMinimumSize(1100, 700);
    resize(1400, 860);

    // ── Load scenarios ────────────────────────────────────────────────────
    QString filePath = scenariosFile.isEmpty()
                           ? ScenarioLoader::findDefaultPath()
                           : scenariosFile;

    if (filePath.isEmpty() || !m_loader.load(filePath))
    {
        QString err = m_loader.lastError();
        LOG_WARNING("TestHarnessWindow: could not load scenarios — {}",
                    err.toStdString());
        // Not fatal — the window opens with an empty picker
    }

    buildUI();
    applyStyles();
    populatePicker();

    LOG_INFO("TestHarnessWindow ready — {} scenario(s) loaded",
             m_loader.scenarios().size());
}

// ── UI Construction ───────────────────────────────────────────────────────────

void TestHarnessWindow::buildUI()
{
    // ── Toolbar ───────────────────────────────────────────────────────────
    auto* toolbar = addToolBar("Scenario");
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);

    toolbar->addWidget(new QLabel("  Scenario: "));

    m_scenarioPicker = new QComboBox;
    m_scenarioPicker->setObjectName("scenarioPicker");
    m_scenarioPicker->setMinimumWidth(300);
    connect(m_scenarioPicker, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TestHarnessWindow::onScenarioChanged);
    toolbar->addWidget(m_scenarioPicker);

    toolbar->addSeparator();

    // "Open file…" button — lets you point at a different scenarios JSON
    auto* openBtn = new QPushButton("📂  Load file…");
    openBtn->setFlat(true);
    openBtn->setToolTip("Load a different test_scenarios.json file");
    connect(openBtn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(
            this, "Open Scenarios File", {},
            "JSON files (*.json);;All files (*)");
        if (!path.isEmpty())
        {
            if (m_loader.load(path))
            {
                populatePicker();
            }
            else
            {
                QMessageBox::warning(this, "Load Failed",
                    QString("Could not load scenarios:\n%1")
                        .arg(m_loader.lastError()));
            }
        }
    });
    toolbar->addWidget(openBtn);

    // ── Central splitter ──────────────────────────────────────────────────
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setObjectName("mainSplitter");
    m_splitter->setHandleWidth(2);

    m_injector  = new UnitTestInjector(this);
    m_extractor = new UnitTestExtractor(this);

    m_splitter->addWidget(m_injector);
    m_splitter->addWidget(m_extractor);
    m_splitter->setSizes({680, 680});
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    setCentralWidget(m_splitter);

    // ── Status bar ────────────────────────────────────────────────────────
    m_statusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusLabel);
    statusBar()->addPermanentWidget(new QLabel("Devana Test Harness  "));

    connect(m_injector,  &UnitTestInjector::injected,
            this, &TestHarnessWindow::onInjected);
    connect(m_extractor, &UnitTestExtractor::messageReceived,
            this, &TestHarnessWindow::onMessageReceived);
}

// ── Scenario picker ───────────────────────────────────────────────────────────

void TestHarnessWindow::populatePicker()
{
    // Populate the picker with signals blocked to suppress spurious
    // onScenarioChanged calls during addItem() / setCurrentIndex().
    // The blocker is scoped so it releases before we manually trigger
    // onScenarioChanged below.
    int selectedIdx = 0;
    {
        QSignalBlocker blocker(m_scenarioPicker);
        m_scenarioPicker->clear();

        const QStringList names = m_loader.scenarioNames();
        for (const QString& n : names)
            m_scenarioPicker->addItem(n);

        if (names.isEmpty())
        {
            m_statusLabel->setText("⚠  No scenarios loaded");
            return;
        }

        // Select requested scenario, fall back to first
        if (!m_initialScenario.isEmpty())
        {
            int found = m_scenarioPicker->findText(m_initialScenario,
                                                   Qt::MatchFixedString | Qt::MatchCaseSensitive);
            if (found >= 0)
                selectedIdx = found;
            else
                LOG_WARNING("Scenario '{}' not found — defaulting to first",
                            m_initialScenario.toStdString());
        }

        m_scenarioPicker->setCurrentIndex(selectedIdx);
    } // QSignalBlocker releases here

    onScenarioChanged(selectedIdx); // apply immediately, signals now unblocked
}

void TestHarnessWindow::onScenarioChanged(int index)
{
    if (index < 0 || index >= m_loader.scenarios().size()) return;

    const Scenario& s = m_loader.scenarios().at(index);
    applyScenario(s);
}

void TestHarnessWindow::applyScenario(const Scenario& s)
{
    if (!s.injector.isNull())
    {
        m_injector->setSchema(s.injector.topic, s.injector.fields);
        m_injector->setEndpoint(s.injector.endpoint);
    }

    if (!s.extractor.isNull())
    {
        m_extractor->setSchema(s.extractor.topic, s.extractor.fields);
        m_extractor->setEndpoint(s.extractor.endpoint);
    }

    m_statusLabel->setText(QString("Loaded: %1").arg(s.name));
    LOG_INFO("Scenario applied: {}", s.name.toStdString());
}

// ── Status slots ──────────────────────────────────────────────────────────────

void TestHarnessWindow::onInjected(const QString& topic, const QString&)
{
    m_statusLabel->setText(
        QString("➤ Injected on %1 at %2")
            .arg(topic, QDateTime::currentDateTime().toString("HH:mm:ss.zzz")));
}

void TestHarnessWindow::onMessageReceived(const QString& topic, const QVariantMap& fields)
{
    m_statusLabel->setText(
        QString("⬇ Received on %1 — %2 fields at %3")
            .arg(topic)
            .arg(fields.size())
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz")));
}

// ── Styles ────────────────────────────────────────────────────────────────────

void TestHarnessWindow::applyStyles()
{
    qApp->setStyleSheet(R"(
        QMainWindow, QWidget {
            background-color: #1a1a2e;
            color: #e0e0f0;
            font-family: "Segoe UI", "SF Pro Text", system-ui, sans-serif;
            font-size: 13px;
        }
        QToolBar {
            background-color: #16213e;
            border-bottom: 1px solid #0f3460;
            padding: 4px;
            spacing: 6px;
        }
        QComboBox#scenarioPicker {
            background-color: #0f3460;
            color: #e0e0f0;
            border: 1px solid #2a5298;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QComboBox#scenarioPicker QAbstractItemView {
            background-color: #0f3460;
            color: #e0e0f0;
            selection-background-color: #2a5298;
        }
        QPushButton[flat="true"] {
            background: transparent;
            color: #7eb8f7;
            border: none;
            padding: 4px 8px;
        }
        QPushButton[flat="true"]:hover { color: #ffffff; }
        QSplitter::handle { background-color: #0f3460; }
        QWidget#injectorHeader, QWidget#extractorHeader {
            background-color: #16213e;
            border-bottom: 1px solid #0f3460;
        }
        QLabel#panelTitle {
            font-size: 14px; font-weight: bold;
            letter-spacing: 1px; color: #7eb8f7;
        }
        QLabel#topicLabel  { font-size: 12px; color: #8888aa; }
        QLabel#schemaHint  { font-size: 11px; color: #555577; }
        QWidget#formContainer  { background-color: #1a1a2e; }
        QScrollArea#fieldScroll { background-color: #1a1a2e; border: none; }
        QLabel { color: #aaaacc; }
        QLabel#fieldValue {
            color: #e0e0f0; font-family: "Courier New", monospace;
            padding: 2px 4px; background-color: #0d0d1a; border-radius: 3px;
        }
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: #0d0d1a; color: #e0e0f0;
            border: 1px solid #2a2a4a; border-radius: 4px; padding: 4px 6px;
            selection-background-color: #2a5298;
        }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
            border-color: #5577cc;
        }
        QCheckBox { color: #aaaacc; }
        QCheckBox::indicator {
            width: 16px; height: 16px; border: 1px solid #2a2a4a;
            border-radius: 3px; background-color: #0d0d1a;
        }
        QCheckBox::indicator:checked { background-color: #3a7bd5; border-color: #5599ff; }
        QPushButton {
            background-color: #0f3460; color: #e0e0f0;
            border: 1px solid #2a5298; border-radius: 4px; padding: 6px 16px;
        }
        QPushButton:hover  { background-color: #2a5298; }
        QPushButton:pressed { background-color: #1a3a70; }
        QPushButton#injectBtn   { background-color: #1a6644; border-color: #2ecc71; font-weight: bold; }
        QPushButton#injectBtn:hover { background-color: #2ecc71; color: #0d0d1a; }
        QPushButton#startStopBtn { background-color: #1a6644; border-color: #2ecc71; font-weight: bold; }
        QPushButton#startStopBtn:hover { background-color: #2ecc71; color: #0d0d1a; }
        QPushButton#burstBtn { background-color: #553300; border-color: #f39c12; }
        QPushButton#burstBtn:hover { background-color: #f39c12; color: #0d0d1a; }
        QPushButton#resetBtn { background-color: #2a2a2a; }
        QPlainTextEdit#logView {
            background-color: #0a0a15; color: #88cc88;
            border: 1px solid #1a1a3a; border-radius: 4px;
            font-family: "Courier New", monospace; font-size: 11px;
        }
        QGroupBox {
            color: #6688aa; border: 1px solid #1a2a4a; border-radius: 4px;
            margin-top: 8px; padding-top: 6px; font-size: 11px;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }
        QWidget#injectorFooter, QWidget#extractorFooter {
            background-color: #16213e; border-top: 1px solid #0f3460;
        }
        QLabel#statusLabel { color: #6688aa; font-size: 11px; }
        QLabel#countLabel  { color: #7eb8f7; font-weight: bold; }
        QFrame#separator   { color: #0f3460; max-height: 1px; }
        QStatusBar {
            background-color: #16213e; color: #6688aa;
            border-top: 1px solid #0f3460; font-size: 11px;
        }
    )");
}
