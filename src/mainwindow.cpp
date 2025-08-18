#include "mainwindow.h"
#include "chezmoiservice.h"
#include "dotfilemanager.h"
#include "configeditor.h"
#include "logger.h"
#include "logviewer.h"

#include <QApplication>
#include <QSplitter>
#include <QTreeView>
#include <QTextEdit>
#include <QListWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

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
    , m_mainSplitter(nullptr)
    , m_fileTreeView(nullptr)
    , m_editorTabs(nullptr)
    , m_statusList(nullptr)
    , m_chezmoiService(new ChezmoiService(this))
    , m_dotfileManager(new DotfileManager(this))
    , m_configEditor(new ConfigEditor(this))
    , m_currentFile()
{
    QIcon appIcon = QIcon::fromTheme(QStringLiteral("dotweaver"), QIcon(QStringLiteral(":/icons/dotweaver.png")));
    setWindowIcon(appIcon);
    
    setupUI();
    setupActions();
    
    connect(m_chezmoiService, &ChezmoiService::operationCompleted,
            this, &MainWindow::refreshFiles);
    connect(m_dotfileManager, &DotfileManager::fileModified,
            this, &MainWindow::onFileModified);
    
    loadDotfiles();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main layout
    auto *mainLayout = new QVBoxLayout(centralWidget);
    
    // Create splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(m_mainSplitter);
    
    // Left panel - File tree
    m_fileTreeView = new QTreeView(this);
    m_fileTreeView->setMinimumWidth(250);
    m_mainSplitter->addWidget(m_fileTreeView);
    
    // Right panel - Editor tabs and status
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    
    m_editorTabs = new QTabWidget(this);
    m_editorTabs->setTabsClosable(true);
    rightLayout->addWidget(m_editorTabs, 3);
    
    m_statusList = new QListWidget(this);
    m_statusList->setMaximumHeight(150);
    rightLayout->addWidget(m_statusList, 1);
    
    m_mainSplitter->addWidget(rightWidget);
    m_mainSplitter->setSizes({250, 600});
    
    setWindowTitle(i18n("Home"));
    resize(1000, 700);
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
    dialog->addPage(m_configEditor, i18n("General"), QStringLiteral("configure"));
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
    // TODO: Load dotfiles from chezmoi and populate tree view
    m_statusList->addItem(i18n("Loading dotfiles..."));
}
