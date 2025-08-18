#include "mainwindow.h"
#include "chezmoiservice.h"
#include "dotfilemanager.h"
#include "configeditor.h"
#include "logger.h"
#include "logviewer.h"

#include <memory>
#include <QApplication>
#include <QSplitter>
#include <QTreeView>
#include <QTextEdit>
#include <QListWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QProcess>
#include <QStatusBar>

#include <KAboutApplicationDialog>
#include <KAboutData>

using namespace Qt::Literals::StringLiterals;
#include <QToolBar>
#include <QFileSystemModel>

#include <KActionCollection>
#include <KStandardAction>
#include <KConfigDialog>
#include <KMessageBox>
#include <KLocalizedString>
#include <KNotification>
#include <QIcon>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
    , m_fileTreeView(nullptr)
    , m_editorTabs(nullptr)
    , m_statusLeft(nullptr)
    , m_statusCenter(nullptr)
    , m_statusRight(nullptr)
    , m_chezmoiService(std::make_unique<ChezmoiService>(this))
    , m_dotfileManager(std::make_unique<DotfileManager>(this))
    , m_configEditor(std::make_unique<ConfigEditor>(this))
    , m_currentFile()
{
    QIcon appIcon = QIcon::fromTheme(QStringLiteral("dotweaver"), QIcon(QStringLiteral(":/icons/dotweaver.png")));
    setWindowIcon(appIcon);
    
    setupUI();
    setupActions();
    
    connect(m_chezmoiService.get(), &ChezmoiService::operationCompleted,
            this, &MainWindow::refreshFiles);
    connect(m_dotfileManager.get(), &DotfileManager::fileModified,
            this, &MainWindow::onFileModified);
    
    loadDotfiles();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main horizontal layout (no margins for flush sidebar)
    auto *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Left sidebar - File tree
    m_fileTreeView = new QTreeView(this);
    m_fileTreeView->setMinimumWidth(200);
    m_fileTreeView->setMaximumWidth(400);
    m_fileTreeView->resize(250, m_fileTreeView->height());
    m_fileTreeView->setModel(m_dotfileManager.get());
    
    /* Style the sidebar to look integrated
    m_fileTreeView->setStyleSheet(
        "QTreeView {"
        "    border: none;"
        "    border-right: 1px solid palette(mid);"
        "    background-color: palette(alternate-base);"
        "    selection-background-color: palette(highlight);"
        "}"
        "QTreeView::item {"
        "    padding: 4px 8px;"
        "    border: none;"
        "}"
        "QTreeView::item:hover {"
        "    background-color: palette(midlight);"
        "}"
        "QTreeView::item:selected {"
        "    background-color: palette(highlight);"
        "}"_L1
    );*/
    
    m_fileTreeView->setHeaderHidden(true);
    m_fileTreeView->setIndentation(15);
    m_fileTreeView->setRootIsDecorated(true);
    mainLayout->addWidget(m_fileTreeView);
    
    // Right panel - Editor tabs only
    m_editorTabs = new QTabWidget(this);
    m_editorTabs->setTabsClosable(true);
    
    mainLayout->addWidget(m_editorTabs, 1); // Give the editor tabs stretch priority
    
    // Setup status bar
    setupStatusBar();
    
    setWindowTitle(i18n("Home"));
    resize(1000, 700);
}

void MainWindow::setupStatusBar()
{
    // Create status bar labels
    m_statusLeft = new QLabel(this);
    m_statusCenter = new QLabel(this);
    m_statusRight = new QLabel(this);
    
    // Set initial text
    m_statusLeft->setText(i18n("Ready"));
    m_statusCenter->setText(""_L1);
    
    // Add to status bar
    statusBar()->addWidget(m_statusLeft, 1);
    statusBar()->addPermanentWidget(m_statusCenter, 1);
    statusBar()->addPermanentWidget(m_statusRight, 0);
    
    /*
    m_statusLeft->setStyleSheet(QStringLiteral(
        "QLabel {"
        "    padding: 2px 8px;"
        "    background-color: palette(highlight);"
        "    color: palette(highlighted-text);"
        "    border-radius: 3px;"
        "    font-family: monospace;"
        "    font-size: 11px;"
        "}"
    ));*/
    
    // Initial git status update
    updateGitStatus();
}

void MainWindow::updateGitStatus()
{
    QProcess gitProcess;
    gitProcess.setWorkingDirectory(m_chezmoiService->getChezmoiDirectory());
    
    // short commit hash and relative time
    gitProcess.start("git"_L1, {
        "log"_L1, "-1"_L1, "--format=%h|%ar"_L1
    });
    
    if (gitProcess.waitForFinished(3000) && gitProcess.exitCode() == 0) {
        QString output = QString::fromUtf8(gitProcess.readAllStandardOutput()).trimmed();
        QStringList parts = output.split('|'_L1);
        
        if (parts.size() == 2) {
            QString hash = parts[0];
            QString timeAgo = parts[1];
            m_statusLeft->setText(QStringLiteral("%1 • %2").arg(hash, timeAgo));
        } else {
            m_statusLeft->setText(i18n("Git: No commits"));
        }
    } else {
        m_statusLeft->setText(i18n("Git: Not available"));
    }
}

void MainWindow::setupActions()
{
    // File menu actions
    KStandardAction::quit(qApp, &QApplication::closeAllWindows, actionCollection());
    
    auto *refreshAction = actionCollection()->addAction(QStringLiteral("refresh"));
    refreshAction->setText(i18n("&Refresh Files"));
    refreshAction->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    KActionCollection::setDefaultShortcut(refreshAction, QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::refreshFiles);
    
    auto *syncAction = actionCollection()->addAction(QStringLiteral("sync"));
    syncAction->setText(i18n("&Sync Files"));
    syncAction->setIcon(QIcon::fromTheme(QStringLiteral("folder-sync")));
    KActionCollection::setDefaultShortcut(syncAction, QKeySequence(Qt::CTRL | Qt::Key_S));
    connect(syncAction, &QAction::triggered, this, &MainWindow::syncFiles);
    
    // View menu
    auto *toggleSidebarAction = actionCollection()->addAction(QStringLiteral("toggle_sidebar"));
    toggleSidebarAction->setText(i18n("Toggle &Sidebar"));
    toggleSidebarAction->setIcon(QIcon::fromTheme(QStringLiteral("view-sidetree")));
    toggleSidebarAction->setCheckable(true);
    toggleSidebarAction->setChecked(true);
    KActionCollection::setDefaultShortcut(toggleSidebarAction, QKeySequence(Qt::CTRL | Qt::Key_B));
    connect(toggleSidebarAction, &QAction::triggered, this, &MainWindow::toggleSidebar);
    
    // Settings menu
    KStandardAction::preferences(this, &MainWindow::openSettings, actionCollection());
    
    // Tools menu
    auto *showLogAction = actionCollection()->addAction(QStringLiteral("show_log"));
    showLogAction->setText(i18n("View &Log..."));
    showLogAction->setIcon(QIcon::fromTheme(QStringLiteral("utilities-log-viewer")));
    showLogAction->setToolTip(i18n("View application log messages"));
    connect(showLogAction, &QAction::triggered, this, &MainWindow::showLogViewer);
    
    // Help menu
    KStandardAction::aboutApp(this, &MainWindow::showAbout, actionCollection());
    
    // Load UI from embedded resource
    setupGUI(Default, QStringLiteral(":/src/ui/dotweaverui.rc"));
}

void MainWindow::openSettings()
{
    if (KConfigDialog::showDialog(QStringLiteral("settings"))) {
        return;
    }
    
    auto *dialog = new KConfigDialog(this, QStringLiteral("settings"), nullptr);
    dialog->addPage(m_configEditor.get(), i18n("General"), QStringLiteral("configure"));
    dialog->show();
}

void MainWindow::refreshFiles()
{
    loadDotfiles();
}

void MainWindow::syncFiles()
{
    LOG_INFO("Starting file sync"_L1);
    m_chezmoiService->applyChanges();
}

void MainWindow::toggleSidebar()
{
    if (m_fileTreeView) {
        bool isVisible = m_fileTreeView->isVisible();
        m_fileTreeView->setVisible(!isVisible);
        
        // Update the action state
        auto *action = actionCollection()->action(QStringLiteral("toggle_sidebar"));
        if (action) {
            action->setChecked(!isVisible);
        }
    }
}

void MainWindow::showAbout()
{
    KAboutApplicationDialog dialog(KAboutData::applicationData(), this);
    dialog.exec();
}

void MainWindow::showLogViewer()
{
    auto *logViewer = new LogViewer(this);
    logViewer->setAttribute(Qt::WA_DeleteOnClose);
    logViewer->show();
    logViewer->raise();
    logViewer->activateWindow();
    
    LOG_INFO("Log viewer opened"_L1);
}

void MainWindow::onFileSelected(const QString &filePath)
{
    m_currentFile = filePath;
    // TODO: Open file in editor tab
}

void MainWindow::onFileModified()
{
    // TODO: Handle file modification
    LOG_INFO("File modified"_L1);
}

void MainWindow::loadDotfiles()
{
    LOG_INFO("MainWindow: Loading dotfiles..."_L1);
    
    // Update status
    if (m_statusRight) {
        m_statusLeft->setText(i18n("Loading dotfiles..."));
    }
    
    // Set up the connection between services
    m_dotfileManager->setChezmoiService(m_chezmoiService.get());
    
    // Refresh the file tree
    m_dotfileManager->refreshFiles();
    
    // Update status and git info
    if (m_statusRight) {
        m_statusRight->setText(i18n("Ready"));
    }
    updateGitStatus();
}
