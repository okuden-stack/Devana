/**
 * @file    UnitTestExtractor.h
 * @brief   Generic ZeroMQ message extractor widget for test harness
 * @author  AK
 *
 * Call setSchema() with a TestSchema to populate the live field display.
 * The widget subscribes to a ZMQ topic on a background thread and
 * updates the UI whenever a message arrives.
 */

#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QLabel>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QFrame>
#include <QMap>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include <atomic>

#include "TestField.h"

// ── Background subscriber thread ──────────────────────────────────────────────

/**
 * @class ZmqSubscriberThread
 * @brief Runs a blocking ZMQ SUB recv loop on a background thread
 *        and emits messageReceived() into the Qt event loop.
 */
class ZmqSubscriberThread : public QThread
{
    Q_OBJECT

public:
    explicit ZmqSubscriberThread(QObject* parent = nullptr);
    ~ZmqSubscriberThread() override;

    void configure(const QString& endpoint, const QString& topicFilter);
    void stop();

signals:
    void messageReceived(const QString& topic, const QString& payload);
    void connectionStateChanged(bool connected, const QString& info);

protected:
    void run() override;

private:
    QString       m_endpoint;
    QString       m_topicFilter;
    std::atomic<bool> m_stop{false};
    QMutex        m_configMutex;
};

// ── Extractor widget ──────────────────────────────────────────────────────────

/**
 * @class UnitTestExtractor
 * @brief Renders a dynamic field display from a TestSchema and
 *        shows live values from incoming ZeroMQ messages.
 *
 * Usage:
 *   auto* extractor = new UnitTestExtractor(this);
 *   extractor->setSchema("devana.threat", threatSchema);
 */
class UnitTestExtractor : public QWidget
{
    Q_OBJECT

public:
    explicit UnitTestExtractor(QWidget* parent = nullptr);
    ~UnitTestExtractor() override;

    /**
     * @brief Rebuild the field display for a new schema.
     * @param topic   ZeroMQ subscription filter (e.g. "devana.threat")
     * @param schema  Ordered list of field descriptors
     */
    void setSchema(const QString& topic, const TestSchema& schema);

    /** @brief Change the ZMQ endpoint and reconnect. */
    void setEndpoint(const QString& endpoint);

    /** @brief Start/stop the background subscriber. */
    void startListening();
    void stopListening();

    /** @brief Return the last N received payloads (raw JSON strings). */
    QStringList capturedMessages(int last = -1) const;

    /** @brief Clear the captured log. */
    void clearLog();

    /** @brief Total messages received since last clear. */
    int messageCount() const { return m_messageCount; }

signals:
    /** Emitted on every received message with parsed field values. */
    void messageReceived(const QString& topic, const QVariantMap& fields);

private slots:
    void onMessageReceived(const QString& topic, const QString& payload);
    void onConnectionStateChanged(bool connected, const QString& info);
    void onStartStopClicked();
    void onClearClicked();
    void onSaveClicked();

private:
    void buildUI();
    void buildFieldRows();
    void clearFieldRows();
    void updateFieldDisplay(const QVariantMap& values);
    void appendToLog(const QString& topic, const QString& payload);

    // ── Schema ───────────────────────────────────────────────────────────────
    QString    m_topic;
    TestSchema m_schema;

    // ── State ────────────────────────────────────────────────────────────────
    int         m_messageCount = 0;
    QStringList m_capturedLog;
    int         m_maxLogEntries = 500;

    // ── Background thread ─────────────────────────────────────────────────────
    ZmqSubscriberThread* m_thread = nullptr;

    // ── Field display widgets (keyed by field name) ───────────────────────────
    QMap<QString, QLabel*> m_fieldValueLabels;

    // ── Layout skeleton ───────────────────────────────────────────────────────
    QFormLayout*   m_formLayout      = nullptr;
    QWidget*       m_formContainer   = nullptr;
    QLabel*        m_statusLabel     = nullptr;
    QLabel*        m_topicLabel      = nullptr;
    QLabel*        m_countLabel      = nullptr;
    QLabel*        m_schemaHint      = nullptr;
    QLineEdit*     m_endpointEdit    = nullptr;
    QPushButton*   m_startStopBtn    = nullptr;
    QPushButton*   m_clearBtn        = nullptr;
    QPushButton*   m_saveBtn         = nullptr;
    QPlainTextEdit* m_logView        = nullptr;
    QSpinBox*      m_maxLogSpin      = nullptr;
    bool           m_listening       = false;
};
