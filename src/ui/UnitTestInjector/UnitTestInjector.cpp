/**
 * @file    UnitTestInjector.cpp
 * @brief   Generic ZeroMQ message injector implementation
 * @author  AK
 */

#include "UnitTestInjector.h"
#include <Logger.h>

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSizePolicy>
#include <QFrame>
#include <zmq.h>

using namespace devana::logging;

// ── Helpers ───────────────────────────────────────────────────────────────────

static QWidget* makeFieldWidget(const TestField& field)
{
    switch (field.type)
    {
    case TestField::Type::String:
    {
        auto* w = new QLineEdit;
        w->setText(field.defaultValue.toString());
        w->setPlaceholderText(field.name);
        return w;
    }
    case TestField::Type::Int:
    {
        auto* w = new QSpinBox;
        w->setRange(static_cast<int>(field.minVal), static_cast<int>(field.maxVal));
        w->setValue(field.defaultValue.toInt());
        w->setSingleStep(1);
        return w;
    }
    case TestField::Type::UInt64:
    {
        // QSpinBox can't hold uint64 — use a line edit with validator hint
        auto* w = new QLineEdit;
        w->setText(field.defaultValue.toString());
        w->setPlaceholderText("uint64");
        return w;
    }
    case TestField::Type::Real:
    {
        auto* w = new QDoubleSpinBox;
        w->setRange(field.minVal, field.maxVal);
        w->setDecimals(field.decimals);
        w->setValue(field.defaultValue.toDouble());
        w->setSingleStep(std::pow(10.0, -field.decimals));
        return w;
    }
    case TestField::Type::Bool:
    {
        auto* w = new QCheckBox;
        w->setChecked(field.defaultValue.toBool());
        return w;
    }
    case TestField::Type::Choice:
    {
        auto* w = new QComboBox;
        w->addItems(field.choices);
        int idx = field.choices.indexOf(field.defaultValue.toString());
        if (idx >= 0) w->setCurrentIndex(idx);
        return w;
    }
    case TestField::Type::ReadOnly:
    {
        auto* w = new QLabel("—");
        w->setStyleSheet("color: #888; font-style: italic;");
        return w;
    }
    }
    Q_UNREACHABLE();
}

static QVariant readFieldValue(const TestField& field, QWidget* widget)
{
    switch (field.type)
    {
    case TestField::Type::String:
        return qobject_cast<QLineEdit*>(widget)->text();
    case TestField::Type::Int:
        return qobject_cast<QSpinBox*>(widget)->value();
    case TestField::Type::UInt64:
        return qobject_cast<QLineEdit*>(widget)->text().toULongLong();
    case TestField::Type::Real:
        return qobject_cast<QDoubleSpinBox*>(widget)->value();
    case TestField::Type::Bool:
        return qobject_cast<QCheckBox*>(widget)->isChecked();
    case TestField::Type::Choice:
        return qobject_cast<QComboBox*>(widget)->currentText();
    case TestField::Type::ReadOnly:
        return QVariant{};
    }
    return {};
}

// ── Constructor / Destructor ─────────────────────────────────────────────────

UnitTestInjector::UnitTestInjector(QWidget* parent)
    : QWidget(parent)
{
    m_zmqContext = zmq_ctx_new();
    m_burstTimer = new QTimer(this);
    connect(m_burstTimer, &QTimer::timeout, this, &UnitTestInjector::onBurstTick);
    buildUI();
    LOG_DEBUG("UnitTestInjector created");
}

UnitTestInjector::~UnitTestInjector()
{
    m_burstTimer->stop();
    if (m_zmqSocket) { zmq_close(m_zmqSocket); m_zmqSocket = nullptr; }
    if (m_zmqContext) { zmq_ctx_destroy(m_zmqContext); m_zmqContext = nullptr; }
}

// ── Public API ────────────────────────────────────────────────────────────────

void UnitTestInjector::setSchema(const QString& topic, const TestSchema& schema)
{
    m_topic  = topic;
    m_schema = schema;

    m_topicLabel->setText(QString("Topic: <b>%1</b>").arg(topic));
    m_schemaHint->setText(QString("%1 field(s)").arg(schema.size()));

    buildFieldRows();
    LOG_INFO("UnitTestInjector schema set — topic: {}, fields: {}",
             topic.toStdString(), schema.size());
}

void UnitTestInjector::setEndpoint(const QString& endpoint)
{
    m_endpoint = endpoint;
    m_endpointEdit->setText(endpoint);
    m_bound = false;

    if (m_zmqSocket)
    {
        zmq_close(m_zmqSocket);
        m_zmqSocket = nullptr;
    }
}

QVariantMap UnitTestInjector::currentValues() const
{
    QVariantMap out;
    for (const auto& field : m_schema)
    {
        QWidget* w = m_fieldWidgets.value(field.name, nullptr);
        if (w)
            out[field.name] = readFieldValue(field, w);
    }
    return out;
}

void UnitTestInjector::setValue(const QString& fieldName, const QVariant& value)
{
    QWidget* w = m_fieldWidgets.value(fieldName, nullptr);
    if (!w) return;

    for (const auto& field : m_schema)
    {
        if (field.name != fieldName) continue;

        switch (field.type)
        {
        case TestField::Type::String:
            qobject_cast<QLineEdit*>(w)->setText(value.toString()); break;
        case TestField::Type::Int:
            qobject_cast<QSpinBox*>(w)->setValue(value.toInt()); break;
        case TestField::Type::UInt64:
            qobject_cast<QLineEdit*>(w)->setText(value.toString()); break;
        case TestField::Type::Real:
            qobject_cast<QDoubleSpinBox*>(w)->setValue(value.toDouble()); break;
        case TestField::Type::Bool:
            qobject_cast<QCheckBox*>(w)->setChecked(value.toBool()); break;
        case TestField::Type::Choice:
        {
            auto* cb = qobject_cast<QComboBox*>(w);
            int idx = cb->findText(value.toString());
            if (idx >= 0) cb->setCurrentIndex(idx);
            break;
        }
        default: break;
        }
        break;
    }
}

void UnitTestInjector::resetToDefaults()
{
    for (const auto& field : m_schema)
        setValue(field.name, field.defaultValue);
}

// ── UI Construction ───────────────────────────────────────────────────────────

void UnitTestInjector::buildUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Header ────────────────────────────────────────────────────────────
    auto* header = new QWidget;
    header->setObjectName("injectorHeader");
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(16, 12, 16, 12);

    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel("⬆  INJECTOR");
    title->setObjectName("panelTitle");
    m_schemaHint = new QLabel("no schema loaded");
    m_schemaHint->setObjectName("schemaHint");
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(m_schemaHint);
    headerLayout->addLayout(titleRow);

    m_topicLabel = new QLabel("Topic: —");
    m_topicLabel->setObjectName("topicLabel");
    headerLayout->addWidget(m_topicLabel);

    // Endpoint row
    auto* endpointRow = new QHBoxLayout;
    endpointRow->addWidget(new QLabel("Endpoint:"));
    m_endpointEdit = new QLineEdit(m_endpoint);
    m_endpointEdit->setObjectName("endpointEdit");
    connect(m_endpointEdit, &QLineEdit::editingFinished, this, [this]() {
        setEndpoint(m_endpointEdit->text().trimmed());
    });
    endpointRow->addWidget(m_endpointEdit, 1);
    headerLayout->addLayout(endpointRow);

    root->addWidget(header);

    // ── Separator ─────────────────────────────────────────────────────────
    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setObjectName("separator");
    root->addWidget(sep);

    // ── Scrollable field area ─────────────────────────────────────────────
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setObjectName("fieldScroll");

    m_formContainer = new QWidget;
    m_formContainer->setObjectName("formContainer");
    m_formLayout = new QFormLayout(m_formContainer);
    m_formLayout->setContentsMargins(16, 12, 16, 12);
    m_formLayout->setSpacing(8);
    m_formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    scroll->setWidget(m_formContainer);
    root->addWidget(scroll, 1);

    // ── Separator ─────────────────────────────────────────────────────────
    auto* sep2 = new QFrame;
    sep2->setFrameShape(QFrame::HLine);
    sep2->setObjectName("separator");
    root->addWidget(sep2);

    // ── Controls footer ───────────────────────────────────────────────────
    auto* footer = new QWidget;
    footer->setObjectName("injectorFooter");
    auto* footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(16, 10, 16, 10);
    footerLayout->setSpacing(8);

    // Burst controls
    auto* burstRow = new QHBoxLayout;
    burstRow->addWidget(new QLabel("Burst:"));
    m_burstCount = new QSpinBox;
    m_burstCount->setRange(1, 10000);
    m_burstCount->setValue(10);
    m_burstCount->setSuffix(" msgs");
    burstRow->addWidget(m_burstCount);
    burstRow->addWidget(new QLabel("every"));
    m_burstInterval = new QSpinBox;
    m_burstInterval->setRange(1, 10000);
    m_burstInterval->setValue(50);
    m_burstInterval->setSuffix(" ms");
    burstRow->addWidget(m_burstInterval);
    burstRow->addStretch();
    footerLayout->addLayout(burstRow);

    // Action buttons
    auto* btnRow = new QHBoxLayout;
    m_resetBtn = new QPushButton("↺ Reset");
    m_resetBtn->setObjectName("resetBtn");
    m_burstBtn = new QPushButton("⚡ Burst");
    m_burstBtn->setObjectName("burstBtn");
    m_injectBtn = new QPushButton("➤ Inject");
    m_injectBtn->setObjectName("injectBtn");
    m_injectBtn->setDefault(true);

    connect(m_resetBtn,  &QPushButton::clicked, this, &UnitTestInjector::onResetClicked);
    connect(m_burstBtn,  &QPushButton::clicked, this, &UnitTestInjector::onBurstClicked);
    connect(m_injectBtn, &QPushButton::clicked, this, &UnitTestInjector::onInjectClicked);

    btnRow->addWidget(m_resetBtn);
    btnRow->addWidget(m_burstBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_injectBtn);
    footerLayout->addLayout(btnRow);

    // Status
    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setObjectName("statusLabel");
    footerLayout->addWidget(m_statusLabel);

    root->addWidget(footer);
}

void UnitTestInjector::buildFieldRows()
{
    clearFieldRows();

    for (const auto& field : m_schema)
    {
        QWidget* w = makeFieldWidget(field);
        w->setObjectName("fieldWidget");
        if (!field.tooltip.isEmpty())
            w->setToolTip(field.tooltip);

        m_fieldWidgets[field.name] = w;

        QString labelText = field.name;
        if (field.required) labelText += " <font color='#ee4466'>*</font>";
        auto* label = new QLabel(labelText);
        label->setTextFormat(Qt::RichText);
        if (!field.tooltip.isEmpty())
            label->setToolTip(field.tooltip);

        m_formLayout->addRow(label, w);
    }
}

void UnitTestInjector::clearFieldRows()
{
    m_fieldWidgets.clear();
    while (m_formLayout->rowCount() > 0)
        m_formLayout->removeRow(0);
}

// ── ZMQ ───────────────────────────────────────────────────────────────────────

bool UnitTestInjector::bindSocket()
{
    if (m_bound) return true;

    if (m_zmqSocket)
    {
        zmq_close(m_zmqSocket);
        m_zmqSocket = nullptr;
    }

    m_zmqSocket = zmq_socket(m_zmqContext, ZMQ_PUB);
    if (!m_zmqSocket)
    {
        m_statusLabel->setText("❌ Failed to create ZMQ socket");
        return false;
    }

    std::string ep = m_endpointEdit->text().trimmed().toStdString();
    if (zmq_bind(m_zmqSocket, ep.c_str()) != 0)
    {
        m_statusLabel->setText(QString("❌ Bind failed: %1").arg(zmq_strerror(errno)));
        zmq_close(m_zmqSocket);
        m_zmqSocket = nullptr;
        return false;
    }

    m_bound = true;
    LOG_INFO("UnitTestInjector bound to {}", ep);
    return true;
}

// ── Serialisation ─────────────────────────────────────────────────────────────

QString UnitTestInjector::serialiseToJson() const
{
    QJsonObject obj;
    for (const auto& field : m_schema)
    {
        QWidget* w = m_fieldWidgets.value(field.name, nullptr);
        if (!w) continue;

        QVariant val = readFieldValue(field, w);

        switch (field.type)
        {
        case TestField::Type::String:
        case TestField::Type::Choice:
        case TestField::Type::UInt64:
            obj[field.name] = val.toString(); break;
        case TestField::Type::Int:
            obj[field.name] = val.toInt(); break;
        case TestField::Type::Real:
            obj[field.name] = val.toDouble(); break;
        case TestField::Type::Bool:
            obj[field.name] = val.toBool(); break;
        case TestField::Type::ReadOnly:
            break;
        }
    }

    // Always stamp a send timestamp
    obj["_injected_at_ms"] = QString::number(
        QDateTime::currentMSecsSinceEpoch());

    return QString::fromUtf8(
        QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void UnitTestInjector::onInjectClicked()
{
    if (m_topic.isEmpty())
    {
        m_statusLabel->setText("⚠  No schema loaded");
        return;
    }

    if (!bindSocket()) return;

    QString payload = serialiseToJson();
    std::string topic   = m_topic.toStdString();
    std::string payloadS = payload.toStdString();

    zmq_send(m_zmqSocket, topic.c_str(),    topic.size(),    ZMQ_SNDMORE);
    zmq_send(m_zmqSocket, payloadS.c_str(), payloadS.size(), 0);

    m_statusLabel->setText(
        QString("✓ Injected at %1")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz")));

    emit injected(m_topic, payload);
    LOG_DEBUG("Injected on topic {}: {}", topic, payloadS);
}

void UnitTestInjector::onBurstClicked()
{
    if (m_burstTimer->isActive())
    {
        m_burstTimer->stop();
        m_burstBtn->setText("⚡ Burst");
        m_statusLabel->setText("⏹ Burst stopped");
        return;
    }

    if (m_topic.isEmpty())
    {
        m_statusLabel->setText("⚠  No schema loaded");
        return;
    }

    m_burstRemaining = m_burstCount->value();
    m_burstBtn->setText("⏹ Stop");
    m_burstTimer->start(m_burstInterval->value());
}

void UnitTestInjector::onBurstTick()
{
    if (m_burstRemaining <= 0)
    {
        m_burstTimer->stop();
        m_burstBtn->setText("⚡ Burst");
        m_statusLabel->setText(
            QString("✓ Burst complete — %1 msgs sent").arg(m_burstCount->value()));
        return;
    }

    onInjectClicked();
    --m_burstRemaining;

    m_statusLabel->setText(
        QString("⚡ Bursting… %1 remaining").arg(m_burstRemaining));
}

void UnitTestInjector::onResetClicked()
{
    resetToDefaults();
    m_statusLabel->setText("↺ Reset to defaults");
}
