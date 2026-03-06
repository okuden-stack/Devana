/**
 * @file    UnitTestInjector.h
 * @brief   Generic ZeroMQ message injector widget for test harness
 * @author  AK
 *
 * Call setSchema() with a TestSchema to populate the form.
 * The widget renders the appropriate input control for each field type,
 * collects values on Inject, serialises to JSON, and publishes to ZMQ.
 */

#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTimer>
#include <QMap>
#include <QVariant>

#include "TestField.h"

/**
 * @class UnitTestInjector
 * @brief Renders a dynamic form from a TestSchema and publishes
 *        serialised messages onto a ZeroMQ PUB socket.
 *
 * Usage:
 *   auto* injector = new UnitTestInjector(this);
 *   injector->setSchema("sentinel.detection", detectionSchema);
 */
class UnitTestInjector : public QWidget
{
    Q_OBJECT

public:
    explicit UnitTestInjector(QWidget* parent = nullptr);
    ~UnitTestInjector() override;

    /**
     * @brief Rebuild the form for a new schema.
     * @param topic   ZeroMQ topic prefix  (e.g. "sentinel.detection")
     * @param schema  Ordered list of field descriptors
     */
    void setSchema(const QString& topic, const TestSchema& schema);

    /**
     * @brief Change the ZMQ endpoint without rebuilding the form.
     * @param endpoint e.g. "tcp://*:5555" or "ipc:///tmp/devana-test"
     */
    void setEndpoint(const QString& endpoint);

    /** @brief Read back current field values as a QVariantMap (name→value). */
    QVariantMap currentValues() const;

    /** @brief Programmatically set a field value. No-op if name not found. */
    void setValue(const QString& fieldName, const QVariant& value);

    /** @brief Reset all fields to their schema defaults. */
    void resetToDefaults();

signals:
    /** Emitted after each successful inject with the serialised JSON payload. */
    void injected(const QString& topic, const QString& jsonPayload);

    /** Emitted on ZMQ or serialisation error. */
    void injectError(const QString& errorMessage);

private slots:
    void onInjectClicked();
    void onBurstClicked();
    void onResetClicked();
    void onBurstTick();

private:
    void buildUI();
    void buildFieldRows();
    void clearFieldRows();
    QString serialiseToJson() const;
    bool bindSocket();

    // ── ZMQ ──────────────────────────────────────────────────────────────────
    void*   m_zmqContext  = nullptr;
    void*   m_zmqSocket   = nullptr;
    QString m_endpoint    = "tcp://*:5555";
    bool    m_bound       = false;

    // ── Schema ───────────────────────────────────────────────────────────────
    QString    m_topic;
    TestSchema m_schema;

    // ── Field widgets (keyed by field name) ──────────────────────────────────
    QMap<QString, QWidget*> m_fieldWidgets;

    // ── Burst state ──────────────────────────────────────────────────────────
    QTimer*  m_burstTimer     = nullptr;
    int      m_burstRemaining = 0;
    QSpinBox* m_burstCount    = nullptr;
    QSpinBox* m_burstInterval = nullptr;

    // ── Layout skeleton (built once in buildUI) ───────────────────────────────
    QFormLayout* m_formLayout    = nullptr;
    QWidget*     m_formContainer = nullptr;
    QLabel*      m_statusLabel   = nullptr;
    QLabel*      m_topicLabel    = nullptr;
    QLineEdit*   m_endpointEdit  = nullptr;
    QPushButton* m_injectBtn     = nullptr;
    QPushButton* m_burstBtn      = nullptr;
    QPushButton* m_resetBtn      = nullptr;
    QLabel*      m_schemaHint    = nullptr;
};
