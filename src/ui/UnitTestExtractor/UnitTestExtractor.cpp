/**
 * @file    UnitTestExtractor.cpp
 * @brief   Generic ZeroMQ message extractor implementation
 * @author  AK
 */

#include "UnitTestExtractor.h"
#include <Logger.h>

#include <QDateTime>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSizePolicy>
#include <QFrame>
#include <zmq.h>

using namespace devana::logging;

// ── ZmqSubscriberThread ───────────────────────────────────────────────────────

ZmqSubscriberThread::ZmqSubscriberThread(QObject* parent)
    : QThread(parent) {}

ZmqSubscriberThread::~ZmqSubscriberThread()
{
    stop();
    wait(2000);
}

void ZmqSubscriberThread::configure(const QString& endpoint, const QString& topicFilter)
{
    QMutexLocker lock(&m_configMutex);
    m_endpoint    = endpoint;
    m_topicFilter = topicFilter;
}

void ZmqSubscriberThread::stop()
{
    m_stop = true;
}

void ZmqSubscriberThread::run()
{
    m_stop = false;

    QString endpoint, filter;
    {
        QMutexLocker lock(&m_configMutex);
        endpoint = m_endpoint;
        filter   = m_topicFilter;
    }

    void* ctx    = zmq_ctx_new();
    void* socket = zmq_socket(ctx, ZMQ_SUB);

    if (!socket)
    {
        emit connectionStateChanged(false, "Failed to create socket");
        zmq_ctx_destroy(ctx);
        return;
    }

    // Set receive timeout so we can check m_stop periodically
    int timeout = 100; // ms
    zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    std::string filterS = filter.toStdString();
    zmq_setsockopt(socket, ZMQ_SUBSCRIBE, filterS.c_str(), filterS.size());

    std::string endpointS = endpoint.toStdString();
    if (zmq_connect(socket, endpointS.c_str()) != 0)
    {
        emit connectionStateChanged(false,
            QString("Connect failed: %1").arg(zmq_strerror(errno)));
        zmq_close(socket);
        zmq_ctx_destroy(ctx);
        return;
    }

    emit connectionStateChanged(true,
        QString("Connected to %1, filter: '%2'").arg(endpoint, filter));
    LOG_INFO("ZmqSubscriberThread connected to {} filter '{}'",
             endpointS, filterS);

    zmq_msg_t topicMsg, payloadMsg;

    while (!m_stop)
    {
        zmq_msg_init(&topicMsg);
        int rc = zmq_msg_recv(&topicMsg, socket, 0);

        if (rc < 0)
        {
            zmq_msg_close(&topicMsg);
            // Timeout is expected — just loop and check m_stop
            if (errno != EAGAIN && errno != ETIMEDOUT)
                LOG_WARNING("ZmqSubscriberThread recv error: {}", zmq_strerror(errno));
            continue;
        }

        QString topic = QString::fromUtf8(
            static_cast<const char*>(zmq_msg_data(&topicMsg)),
            static_cast<int>(zmq_msg_size(&topicMsg)));
        zmq_msg_close(&topicMsg);

        zmq_msg_init(&payloadMsg);
        rc = zmq_msg_recv(&payloadMsg, socket, 0);
        if (rc < 0)
        {
            zmq_msg_close(&payloadMsg);
            continue;
        }

        QString payload = QString::fromUtf8(
            static_cast<const char*>(zmq_msg_data(&payloadMsg)),
            static_cast<int>(zmq_msg_size(&payloadMsg)));
        zmq_msg_close(&payloadMsg);

        emit messageReceived(topic, payload);
    }

    zmq_close(socket);
    zmq_ctx_destroy(ctx);
    emit connectionStateChanged(false, "Disconnected");
    LOG_INFO("ZmqSubscriberThread stopped");
}

// ── UnitTestExtractor ─────────────────────────────────────────────────────────

UnitTestExtractor::UnitTestExtractor(QWidget* parent)
    : QWidget(parent)
{
    m_thread = new ZmqSubscriberThread(this);
    connect(m_thread, &ZmqSubscriberThread::messageReceived,
            this,     &UnitTestExtractor::onMessageReceived,
            Qt::QueuedConnection);
    connect(m_thread, &ZmqSubscriberThread::connectionStateChanged,
            this,     &UnitTestExtractor::onConnectionStateChanged,
            Qt::QueuedConnection);

    buildUI();
    LOG_DEBUG("UnitTestExtractor created");
}

UnitTestExtractor::~UnitTestExtractor()
{
    stopListening();
}

// ── Public API ────────────────────────────────────────────────────────────────

void UnitTestExtractor::setSchema(const QString& topic, const TestSchema& schema)
{
    m_topic  = topic;
    m_schema = schema;

    m_topicLabel->setText(QString("Topic: <b>%1</b>").arg(topic));
    m_schemaHint->setText(QString("%1 field(s)").arg(schema.size()));

    m_thread->configure(
        m_endpointEdit->text().trimmed(),
        topic);

    buildFieldRows();
    LOG_INFO("UnitTestExtractor schema set — topic: {}, fields: {}",
             topic.toStdString(), schema.size());
}

void UnitTestExtractor::setEndpoint(const QString& endpoint)
{
    m_endpointEdit->setText(endpoint);
    m_thread->configure(endpoint, m_topic);
}

void UnitTestExtractor::startListening()
{
    if (m_listening) return;
    m_thread->configure(m_endpointEdit->text().trimmed(), m_topic);
    m_thread->start();
    m_listening = true;
    m_startStopBtn->setText("⏹ Stop");
    m_statusLabel->setText("⏳ Connecting…");
}

void UnitTestExtractor::stopListening()
{
    if (!m_listening) return;
    m_thread->stop();
    m_thread->wait(2000);
    m_listening = false;
    m_startStopBtn->setText("▶ Listen");
    m_statusLabel->setText("⏹ Stopped");
}

QStringList UnitTestExtractor::capturedMessages(int last) const
{
    if (last < 0 || last >= m_capturedLog.size())
        return m_capturedLog;
    return m_capturedLog.mid(m_capturedLog.size() - last);
}

void UnitTestExtractor::clearLog()
{
    m_capturedLog.clear();
    m_messageCount = 0;
    m_logView->clear();
    m_countLabel->setText("0 msgs");
}

// ── UI Construction ───────────────────────────────────────────────────────────

void UnitTestExtractor::buildUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Header ────────────────────────────────────────────────────────────
    auto* header = new QWidget;
    header->setObjectName("extractorHeader");
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(16, 12, 16, 12);

    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel("⬇  EXTRACTOR");
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

    auto* endpointRow = new QHBoxLayout;
    endpointRow->addWidget(new QLabel("Endpoint:"));
    m_endpointEdit = new QLineEdit("tcp://localhost:5555");
    m_endpointEdit->setObjectName("endpointEdit");
    endpointRow->addWidget(m_endpointEdit, 1);
    headerLayout->addLayout(endpointRow);

    root->addWidget(header);

    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setObjectName("separator");
    root->addWidget(sep);

    // ── Live field display ─────────────────────────────────────────────────
    auto* liveGroup = new QGroupBox("Last Received");
    liveGroup->setObjectName("liveGroup");
    auto* liveLayout = new QVBoxLayout(liveGroup);
    liveLayout->setContentsMargins(8, 8, 8, 8);

    auto* liveScroll = new QScrollArea;
    liveScroll->setWidgetResizable(true);
    liveScroll->setFrameShape(QFrame::NoFrame);
    liveScroll->setMaximumHeight(260);

    m_formContainer = new QWidget;
    m_formContainer->setObjectName("formContainer");
    m_formLayout = new QFormLayout(m_formContainer);
    m_formLayout->setContentsMargins(8, 8, 8, 8);
    m_formLayout->setSpacing(6);
    m_formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    liveScroll->setWidget(m_formContainer);
    liveLayout->addWidget(liveScroll);
    root->addWidget(liveGroup);

    // ── Message log ───────────────────────────────────────────────────────
    auto* logGroup = new QGroupBox("Message Log");
    logGroup->setObjectName("logGroup");
    auto* logLayout = new QVBoxLayout(logGroup);
    logLayout->setContentsMargins(8, 8, 8, 8);

    m_logView = new QPlainTextEdit;
    m_logView->setObjectName("logView");
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(500);
    // Font is controlled by the #logView stylesheet rule ("Courier New", 11px)
    logLayout->addWidget(m_logView, 1);

    root->addWidget(logGroup, 1);

    // ── Footer ────────────────────────────────────────────────────────────
    auto* sep2 = new QFrame;
    sep2->setFrameShape(QFrame::HLine);
    sep2->setObjectName("separator");
    root->addWidget(sep2);

    auto* footer = new QWidget;
    footer->setObjectName("extractorFooter");
    auto* footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(16, 10, 16, 10);
    footerLayout->setSpacing(8);

    // Log limit row
    auto* limitRow = new QHBoxLayout;
    limitRow->addWidget(new QLabel("Keep last:"));
    m_maxLogSpin = new QSpinBox;
    m_maxLogSpin->setRange(1, 10000);
    m_maxLogSpin->setValue(500);
    m_maxLogSpin->setSuffix(" msgs");
    connect(m_maxLogSpin, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int v){ m_maxLogEntries = v; });
    limitRow->addWidget(m_maxLogSpin);
    limitRow->addStretch();
    m_countLabel = new QLabel("0 msgs");
    m_countLabel->setObjectName("countLabel");
    limitRow->addWidget(m_countLabel);
    footerLayout->addLayout(limitRow);

    // Action buttons
    auto* btnRow = new QHBoxLayout;
    m_clearBtn    = new QPushButton("🗑 Clear");
    m_saveBtn     = new QPushButton("💾 Save Log");
    m_startStopBtn = new QPushButton("▶ Listen");
    m_startStopBtn->setObjectName("startStopBtn");

    connect(m_clearBtn,     &QPushButton::clicked, this, &UnitTestExtractor::onClearClicked);
    connect(m_saveBtn,      &QPushButton::clicked, this, &UnitTestExtractor::onSaveClicked);
    connect(m_startStopBtn, &QPushButton::clicked, this, &UnitTestExtractor::onStartStopClicked);

    btnRow->addWidget(m_clearBtn);
    btnRow->addWidget(m_saveBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_startStopBtn);
    footerLayout->addLayout(btnRow);

    m_statusLabel = new QLabel("Idle");
    m_statusLabel->setObjectName("statusLabel");
    footerLayout->addWidget(m_statusLabel);

    root->addWidget(footer);
}

void UnitTestExtractor::buildFieldRows()
{
    clearFieldRows();

    for (const auto& field : m_schema)
    {
        auto* valueLabel = new QLabel("—");
        valueLabel->setObjectName("fieldValue");
        valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        m_fieldValueLabels[field.name] = valueLabel;

        QString labelText = field.name;
        if (field.required) labelText += " *";
        auto* label = new QLabel(labelText);
        if (!field.tooltip.isEmpty())
        {
            label->setToolTip(field.tooltip);
            valueLabel->setToolTip(field.tooltip);
        }

        m_formLayout->addRow(label, valueLabel);
    }
}

void UnitTestExtractor::clearFieldRows()
{
    m_fieldValueLabels.clear();
    while (m_formLayout->rowCount() > 0)
        m_formLayout->removeRow(0);
}

void UnitTestExtractor::updateFieldDisplay(const QVariantMap& values)
{
    for (auto it = values.begin(); it != values.end(); ++it)
    {
        QLabel* label = m_fieldValueLabels.value(it.key(), nullptr);
        if (!label) continue;

        QString display = it.value().toString();

        // Colour-code booleans
        if (it.value().typeId() == QMetaType::Bool)
        {
            bool v = it.value().toBool();
            label->setStyleSheet(v ? "color: #2ecc71; font-weight: bold;"
                                   : "color: #e74c3c;");
            display = v ? "true" : "false";
        }
        // Highlight floats near 0 or 1 (common confidence ranges)
        else if (it.value().typeId() == QMetaType::Double)
        {
            double v = it.value().toDouble();
            if (v >= 0.9)
                label->setStyleSheet("color: #e74c3c; font-weight: bold;");
            else if (v >= 0.7)
                label->setStyleSheet("color: #f39c12;");
            else
                label->setStyleSheet("");
        }
        else
        {
            label->setStyleSheet("");
        }

        label->setText(display);
    }
}

void UnitTestExtractor::appendToLog(const QString& topic, const QString& payload)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString entry = QString("[%1] %2\n%3").arg(timestamp, topic, payload);
    m_logView->appendPlainText(entry);
    m_logView->appendPlainText(""); // blank separator

    m_capturedLog.append(payload);
    if (m_capturedLog.size() > m_maxLogEntries)
        m_capturedLog.removeFirst();
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void UnitTestExtractor::onMessageReceived(const QString& topic, const QString& payload)
{
    ++m_messageCount;
    m_countLabel->setText(QString("%1 msgs").arg(m_messageCount));
    m_statusLabel->setText(
        QString("✓ Last: %1")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz")));

    // Parse JSON and update live field display
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8(), &err);

    QVariantMap values;
    if (err.error == QJsonParseError::NoError && doc.isObject())
    {
        QJsonObject obj = doc.object();
        for (auto it = obj.begin(); it != obj.end(); ++it)
            values[it.key()] = it.value().toVariant();
    }
    else
    {
        // Not JSON — treat whole payload as a single "raw" field
        values["raw"] = payload;
    }

    updateFieldDisplay(values);
    appendToLog(topic, payload);

    emit messageReceived(topic, values);
}

void UnitTestExtractor::onConnectionStateChanged(bool connected, const QString& info)
{
    if (connected)
        m_statusLabel->setText(QString("🟢 %1").arg(info));
    else
        m_statusLabel->setText(QString("🔴 %1").arg(info));

    LOG_INFO("UnitTestExtractor connection: {} — {}", connected, info.toStdString());
}

void UnitTestExtractor::onStartStopClicked()
{
    if (m_listening)
        stopListening();
    else
        startListening();
}

void UnitTestExtractor::onClearClicked()
{
    clearLog();
    for (auto& label : m_fieldValueLabels)
    {
        label->setText("—");
        label->setStyleSheet("");
    }
    m_statusLabel->setText("🗑 Log cleared");
}

void UnitTestExtractor::onSaveClicked()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Save Message Log", "",
        "Text files (*.txt);;JSON Lines (*.jsonl);;All files (*)");

    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        m_statusLabel->setText(QString("❌ Save failed: %1").arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    for (const auto& msg : m_capturedLog)
        out << msg << "\n";

    m_statusLabel->setText(
        QString("💾 Saved %1 messages").arg(m_capturedLog.size()));
}
