/**
 * @file    MainWindow.cpp
 * @brief   Sentinel main window implementation
 * @author  AK
 * @date    2026-02-03
 */

#include "MainWindow.h"
#include "Constants.h"
#include <Logger.h>
#include <QApplication>
#include <QDateTime>
#include <QTimer>
#include <QScrollArea>
#include <QFrame>
#include <QGridLayout>

using namespace sentinel::logging;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_centralWidget(nullptr),
      m_mainLayout(nullptr),
      m_navigationPanel(nullptr),
      m_contentStack(nullptr),
      m_currentPage(Page::Dashboard)
{
    LOG_INFO("Initializing Sentinel MainWindow...");

    setWindowTitle("Sentinel");
    setMinimumSize(okuden::MIN_WINDOW_WIDTH, okuden::MIN_WINDOW_HEIGHT);
    resize(1400, 900);

    initializeUI();
    applyStyles();
    navigateToPage(Page::Dashboard);

    LOG_INFO("MainWindow initialization complete");
}

MainWindow::~MainWindow()
{
    LOG_INFO("Shutting down MainWindow");
}

void MainWindow::initializeUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setupNavigation();
    setupContentArea();
    setupStatusBar();
}

void MainWindow::setupNavigation()
{
    m_navigationPanel = new QWidget(this);
    m_navigationPanel->setObjectName("navigationPanel");
    m_navigationPanel->setFixedWidth(220);

    QVBoxLayout *navLayout = new QVBoxLayout(m_navigationPanel);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(0);

    // App header
    QWidget *header = new QWidget(m_navigationPanel);
    header->setObjectName("navHeader");
    header->setFixedHeight(80);

    QVBoxLayout *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *titleLabel = new QLabel("SENTINEL", header);
    titleLabel->setObjectName("appTitle");
    QLabel *subtitleLabel = new QLabel("Intelligence Platform", header);
    subtitleLabel->setObjectName("appSubtitle");

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subtitleLabel);
    navLayout->addWidget(header);

    // Separator
    QFrame *separator = new QFrame(m_navigationPanel);
    separator->setFrameShape(QFrame::HLine);
    separator->setObjectName("separator");
    navLayout->addWidget(separator);

    navLayout->addSpacing(20);

    // Navigation buttons
    m_navButtons[Page::Dashboard] =
        createNavButton("Dashboard", "■", Page::Dashboard);
    navLayout->addWidget(m_navButtons[Page::Dashboard]);

    m_navButtons[Page::Analytics] =
        createNavButton("Analytics", "◆", Page::Analytics);
    navLayout->addWidget(m_navButtons[Page::Analytics]);

    navLayout->addStretch();

    m_navButtons[Page::Settings] =
        createNavButton("Settings", "◉", Page::Settings);
    navLayout->addWidget(m_navButtons[Page::Settings]);

    navLayout->addSpacing(10);

    m_mainLayout->addWidget(m_navigationPanel);
}

void MainWindow::setupContentArea()
{
    m_contentStack = new QStackedWidget(this);
    m_contentStack->setObjectName("contentStack");
    m_mainLayout->addWidget(m_contentStack, 1);
}

void MainWindow::setupStatusBar()
{
    QStatusBar *status = statusBar();
    status->setObjectName("statusBar");

    m_statusLabel = new QLabel("Ready", this);
    status->addWidget(m_statusLabel);

    status->addPermanentWidget(new QLabel(" | ", this));

    QLabel *versionLabel = new QLabel("v1.0.0", this);
    status->addPermanentWidget(versionLabel);

    status->addPermanentWidget(new QLabel(" | ", this));

    QLabel *timeLabel = new QLabel(this);
    status->addPermanentWidget(timeLabel);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [timeLabel]()
            { timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")); });
    timer->start(1000);
    timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
}

void MainWindow::applyStyles()
{
    QString styleSheet = R"(
        QMainWindow {
            background-color: #f5f5f5;
        }

        #navigationPanel {
            background-color: #1e1e2e;
            border-right: 1px solid #2e2e3e;
        }

        #navHeader {
            background-color: #252535;
        }

        #appTitle {
            color: #ffffff;
            font-size: 18px;
            font-weight: bold;
            letter-spacing: 2px;
        }

        #appSubtitle {
            color: #8888aa;
            font-size: 11px;
            letter-spacing: 1px;
        }

        #separator {
            background-color: #2e2e3e;
            max-height: 1px;
            border: none;
        }

        QPushButton[class="navButton"] {
            background-color: transparent;
            color: #aaaacc;
            text-align: left;
            padding: 15px 20px;
            border: none;
            border-left: 3px solid transparent;
            font-size: 14px;
        }

        QPushButton[class="navButton"]:hover {
            background-color: #2a2a3a;
            color: #ffffff;
        }

        QPushButton[class="navButton"]:checked {
            background-color: #2a2a3a;
            color: #ffffff;
            border-left: 3px solid #6c63ff;
        }

        #contentStack {
            background-color: #f5f5f5;
        }

        QWidget[class="page"] {
            background-color: #f5f5f5;
        }

        QWidget[class="card"] {
            background-color: #ffffff;
            border-radius: 8px;
            border: 1px solid #e0e0e0;
        }

        QLabel[class="cardTitle"] {
            color: #2c3e50;
            font-size: 18px;
            font-weight: bold;
        }

        QLabel[class="cardSubtitle"] {
            color: #7f8c8d;
            font-size: 13px;
        }

        QLabel[class="cardContent"] {
            color: #34495e;
            font-size: 14px;
            line-height: 1.6;
        }

        QLabel[class="pageTitle"] {
            color: #2c3e50;
            font-size: 28px;
            font-weight: 300;
        }

        QLabel[class="sectionTitle"] {
            color: #34495e;
            font-size: 16px;
            font-weight: bold;
        }

        #statusBar {
            background-color: #ffffff;
            color: #666666;
            border-top: 1px solid #e0e0e0;
        }

        #statusBar QLabel {
            color: #666666;
        }
    )";

    qApp->setStyleSheet(styleSheet);
}

QPushButton *MainWindow::createNavButton(const QString &text, const QString &icon, Page page)
{
    QPushButton *button = new QPushButton(
        QString("%1  %2").arg(icon, text),
        m_navigationPanel);
    button->setProperty("class", "navButton");
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);

    connect(button, &QPushButton::clicked, this, [this, page]()
            { navigateToPage(page); });

    return button;
}

void MainWindow::navigateToPage(Page page)
{
    m_currentPage = page;

    QWidget *pageWidget = getOrCreatePage(page);

    if (m_contentStack->indexOf(pageWidget) == -1)
    {
        m_contentStack->addWidget(pageWidget);
    }
    m_contentStack->setCurrentWidget(pageWidget);

    updateNavigationButtons();

    QString pageName;
    switch (page)
    {
    case Page::Dashboard:
        pageName = "Dashboard";
        break;
    case Page::Analytics:
        pageName = "Analytics";
        break;
    case Page::Settings:
        pageName = "Settings";
        break;
    }
    m_statusLabel->setText(QString("Viewing: %1").arg(pageName));

    LOG_DEBUG("Navigated to page: {}", pageName.toStdString());
}

void MainWindow::updateNavigationButtons()
{
    for (auto it = m_navButtons.begin(); it != m_navButtons.end(); ++it)
    {
        it->second->setChecked(it->first == m_currentPage);
    }
}

QWidget *MainWindow::getOrCreatePage(Page page)
{
    auto it = m_pages.find(page);
    if (it != m_pages.end())
    {
        return it->second;
    }

    QWidget *pageWidget = nullptr;
    switch (page)
    {
    case Page::Dashboard:
        pageWidget = createDashboardPage();
        break;
    case Page::Analytics:
        pageWidget = createAnalyticsPage();
        break;
    case Page::Settings:
        pageWidget = createSettingsPage();
        break;
    }

    if (pageWidget)
    {
        m_pages[page] = pageWidget;
    }

    return pageWidget;
}

QWidget *MainWindow::createCard(const QString &title, const QString &subtitle, const QString &content)
{
    QWidget *card = new QWidget();
    card->setProperty("class", "card");

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(25, 20, 25, 20);
    cardLayout->setSpacing(10);

    if (!title.isEmpty())
    {
        QLabel *titleLabel = new QLabel(title, card);
        titleLabel->setProperty("class", "cardTitle");
        cardLayout->addWidget(titleLabel);
    }

    if (!subtitle.isEmpty())
    {
        QLabel *subtitleLabel = new QLabel(subtitle, card);
        subtitleLabel->setProperty("class", "cardSubtitle");
        cardLayout->addWidget(subtitleLabel);
    }

    if (!content.isEmpty())
    {
        cardLayout->addSpacing(5);
        QLabel *contentLabel = new QLabel(content, card);
        contentLabel->setProperty("class", "cardContent");
        contentLabel->setWordWrap(true);
        cardLayout->addWidget(contentLabel);
    }

    return card;
}

QWidget *MainWindow::createDashboardPage()
{
    LOG_DEBUG("Creating Dashboard page");

    QWidget *page = new QWidget();
    page->setProperty("class", "page");

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(30);

    QLabel *pageTitle = new QLabel("Dashboard", page);
    pageTitle->setProperty("class", "pageTitle");
    layout->addWidget(pageTitle);

    QWidget *welcomeCard = createCard(
        "Welcome to Sentinel",
        "Intelligence Platform",
        "Your application is ready. This is a clean foundation to build upon. "
        "Add your features, integrate your data sources, and customize the interface to your needs.");
    layout->addWidget(welcomeCard);

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setSpacing(20);

    QWidget *statsCard1 = createCard("System Status", "", "All systems operational");
    QWidget *statsCard2 = createCard("Active Sessions", "", "Ready to connect");
    QWidget *statsCard3 = createCard("Data Sources", "", "Configure in Settings");

    gridLayout->addWidget(statsCard1, 0, 0);
    gridLayout->addWidget(statsCard2, 0, 1);
    gridLayout->addWidget(statsCard3, 0, 2);

    layout->addLayout(gridLayout);
    layout->addStretch();

    return page;
}

QWidget *MainWindow::createAnalyticsPage()
{
    LOG_DEBUG("Creating Analytics page");

    QWidget *page = new QWidget();
    page->setProperty("class", "page");

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(30);

    QLabel *pageTitle = new QLabel("Analytics", page);
    pageTitle->setProperty("class", "pageTitle");
    layout->addWidget(pageTitle);

    QWidget *card = createCard(
        "Analytics Dashboard",
        "Data visualization and insights",
        "This is where you can add charts, graphs, and data analysis tools. "
        "Integrate your analytics libraries and visualizations here.");
    layout->addWidget(card);

    layout->addStretch();

    return page;
}

QWidget *MainWindow::createSettingsPage()
{
    LOG_DEBUG("Creating Settings page");

    QWidget *page = new QWidget();
    page->setProperty("class", "page");

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(30);

    QLabel *pageTitle = new QLabel("Settings", page);
    pageTitle->setProperty("class", "pageTitle");
    layout->addWidget(pageTitle);

    QWidget *generalCard = createCard(
        "General Settings",
        "Application preferences",
        "Configure application behavior, appearance themes, and general preferences here.");
    layout->addWidget(generalCard);

    QWidget *advancedCard = createCard(
        "Advanced Settings",
        "Expert configuration",
        "Advanced options for power users and system administrators.");
    layout->addWidget(advancedCard);

    layout->addStretch();

    return page;
}
