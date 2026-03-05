/**
 * @file    MainWindow.h
 * @brief   Sentinel main window interface
 * @author  AK
 * @date    2026-02-03
 *
 * Modern, clean main window for Sentinel application
 */

#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStatusBar>
#include <map>

/**
 * @enum Page
 * @brief Application pages
 */
enum class Page
{
    Dashboard,
    Analytics,
    Settings
};

/**
 * @class MainWindow
 * @brief Main application window for Sentinel
 *
 * Provides a clean, modern interface with side navigation
 * and content area for different application views
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void navigateToPage(Page page);

private:
    // Setup methods
    void initializeUI();
    void setupNavigation();
    void setupContentArea();
    void setupStatusBar();
    void applyStyles();

    // Page creation
    QWidget *createDashboardPage();
    QWidget *createAnalyticsPage();
    QWidget *createSettingsPage();

    // Helpers
    void updateNavigationButtons();
    QWidget *getOrCreatePage(Page page);
    QPushButton *createNavButton(const QString &text, const QString &icon, Page page);
    QWidget *createCard(const QString &title, const QString &subtitle, const QString &content);

private:
    // Layout components
    QWidget *m_centralWidget;
    QHBoxLayout *m_mainLayout;
    QWidget *m_navigationPanel;
    QStackedWidget *m_contentStack;

    // Navigation
    std::map<Page, QPushButton *> m_navButtons;
    std::map<Page, QWidget *> m_pages;
    Page m_currentPage;

    // Status bar
    QLabel *m_statusLabel;
};
