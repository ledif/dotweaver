#include "mainwindow.h"
#include "chezmoiservice.h"
#include "dotfilemanager.h"
#include "configeditor.h"
#include "filetab.h"
#include "logger.h"
#include "logviewer.h"
#include "dataviewer.h"
#include "statusbar.h"


#include <KAboutApplicationDialog>
#include <KAboutData>
#include <KActionCollection>
#include <KStandardAction>
#include <KConfigDialog>
#include <KMessageBox>
#include <KLocalizedString>
#include <KNotification>
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
#include <QToolBar>
#include <QFileSystemModel>
#include <QIcon>
#include <QStandardPaths>
#include <QMenu>
#include <QPoint>

#include <memory>

using namespace Qt::Literals::StringLiterals;


MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
    , m_fileTreeView(nullptr)
    , m_editorTabs(nullptr)
    , m_splitter(nullptr)
    , m_chezmoiService(std::make_unique<ChezmoiService>(this))
    , m_dotfileManager(std::make_unique<DotfileManager>(this))
    , m_configEditor(std::make_unique<ConfigEditor>(this))
    , m_statusBar(nullptr)
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
    
    // Create main layout
    auto *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Create horizontal splitter
    m_splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left panel - File tree
    m_fileTreeView = new QTreeView(this);
    m_fileTreeView->setMinimumWidth(150);
    m_fileTreeView->setModel(m_dotfileManager.get());
    m_fileTreeView->setHeaderHidden(true);
    m_fileTreeView->setIndentation(15);
    m_fileTreeView->setRootIsDecorated(true);
    
    // Add context menu for expand/collapse
    m_fileTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_fileTreeView, &QTreeView::customContextMenuRequested, 
            this, &MainWindow::showTreeContextMenu);
    
    // Connect file double-click to open in tab
    connect(m_fileTreeView, &QTreeView::doubleClicked,
            this, &MainWindow::onFileDoubleClicked);
    
    // Right panel - Editor tabs
    m_editorTabs = new QTabWidget(this);
    m_editorTabs->setTabsClosable(true);
    m_editorTabs->setMovable(true);
    m_editorTabs->setDocumentMode(true);
    
    // Connect tab close signal
    connect(m_editorTabs, &QTabWidget::tabCloseRequested,
            this, &MainWindow::onTabCloseRequested);
    
    // Add widgets to splitter
    m_splitter->addWidget(m_fileTreeView);
    m_splitter->addWidget(m_editorTabs);
    
    // Set initial splitter sizes (25% for tree, 75% for editor)
    m_splitter->setSizes({250, 750});
    m_splitter->setCollapsible(0, false); // Don't allow tree to collapse completely
    
    // Add splitter to main layout
    mainLayout->addWidget(m_splitter);
    
    // Setup status bar
    setupStatusBar();
    
    setWindowTitle(i18n("Home"));
    resize(1000, 700);
}

void MainWindow::setupStatusBar()
{
    // Create Qt's status bar first
    auto *qtStatusBar = statusBar(); // This creates the QStatusBar if it doesn't exist
    
    // Create our custom StatusBar manager
    m_statusBar = new ::StatusBar(qtStatusBar, m_chezmoiService.get(), this);
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
    
    auto *expandAllAction = actionCollection()->addAction(QStringLiteral("expand_all"));
    expandAllAction->setText(i18n("E&xpand All"));
    expandAllAction->setIcon(QIcon::fromTheme(QStringLiteral("expand-all")));
    expandAllAction->setToolTip(i18n("Expand all items in the file tree"));
    KActionCollection::setDefaultShortcut(expandAllAction, QKeySequence(Qt::CTRL | Qt::Key_Plus));
    connect(expandAllAction, &QAction::triggered, this, &MainWindow::expandAllItems);
    
    auto *collapseAllAction = actionCollection()->addAction(QStringLiteral("collapse_all"));
    collapseAllAction->setText(i18n("&Collapse All"));
    collapseAllAction->setIcon(QIcon::fromTheme(QStringLiteral("collapse-all")));
    collapseAllAction->setToolTip(i18n("Collapse all items in the file tree"));
    KActionCollection::setDefaultShortcut(collapseAllAction, QKeySequence(Qt::CTRL | Qt::Key_Minus));
    connect(collapseAllAction, &QAction::triggered, this, &MainWindow::collapseAllItems);
    
    // Settings menu
    KStandardAction::preferences(this, &MainWindow::openSettings, actionCollection());
    
    // Tools menu
    auto *showLogAction = actionCollection()->addAction(QStringLiteral("show_log"));
    showLogAction->setText(i18n("View &Log..."));
    showLogAction->setIcon(QIcon::fromTheme(QStringLiteral("utilities-log-viewer")));
    showLogAction->setToolTip(i18n("View application log messages"));
    connect(showLogAction, &QAction::triggered, this, &MainWindow::showLogViewer);
    
    auto *viewDataAction = actionCollection()->addAction(QStringLiteral("view_template_data"));
    viewDataAction->setText(i18n("View Template &Data..."));
    viewDataAction->setIcon(QIcon::fromTheme(QStringLiteral("code-variable")));
    viewDataAction->setToolTip(i18n("View chezmoi template variables"));
    connect(viewDataAction, &QAction::triggered, this, &MainWindow::showDataViewer);
    
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

void MainWindow::expandAllItems()
{
    if (m_fileTreeView) {
        m_fileTreeView->expandAll();
    }
}

void MainWindow::collapseAllItems()
{
    if (m_fileTreeView) {
        m_fileTreeView->collapseAll();
    }
}

void MainWindow::showTreeContextMenu(const QPoint &position)
{
    if (!m_fileTreeView) {
        return;
    }
    
    QMenu contextMenu(this);
    
    auto *expandAllAction = contextMenu.addAction(QIcon::fromTheme(QStringLiteral("expand-all")), 
                                                  i18n("Expand All"));
    connect(expandAllAction, &QAction::triggered, this, &MainWindow::expandAllItems);
    
    auto *collapseAllAction = contextMenu.addAction(QIcon::fromTheme(QStringLiteral("collapse-all")), 
                                                    i18n("Collapse All"));
    connect(collapseAllAction, &QAction::triggered, this, &MainWindow::collapseAllItems);
    
    contextMenu.addSeparator();
    
    auto *refreshAction = contextMenu.addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), 
                                                i18n("Refresh"));
    connect(refreshAction, &QAction::triggered, this, &MainWindow::refreshFiles);
    
    contextMenu.exec(m_fileTreeView->mapToGlobal(position));
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

void MainWindow::showDataViewer()
{
    if (!m_chezmoiService) {
        KMessageBox::error(this, i18n("Chezmoi service is not available."));
        return;
    }
    
    QString jsonData = m_chezmoiService->getTemplateData();
    if (jsonData.isEmpty()) {
        KMessageBox::error(this, i18n("Failed to retrieve template data from chezmoi."));
        return;
    }
    
    auto *dataViewer = new DataViewer(jsonData, this);
    dataViewer->setAttribute(Qt::WA_DeleteOnClose);
    dataViewer->show();
    dataViewer->raise();
    dataViewer->activateWindow();
    
    LOG_INFO("Data viewer opened"_L1);
}

void MainWindow::onFileSelected(const QString &filePath)
{
    m_currentFile = filePath;
    LOG_DEBUG(QStringLiteral("File selected: %1").arg(filePath));
}

void MainWindow::onFileDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid() || !m_dotfileManager) {
        return;
    }
    
    // Get the file path from the model
    QString filePath = m_dotfileManager->getFilePath(index);
    if (filePath.isEmpty()) {
        LOG_DEBUG("Double-clicked item has no file path (likely a directory)"_L1);
        return;
    }
    
    // Convert source path to target path for display/editing
    QString targetPath = m_chezmoiService->convertToTargetPath(filePath);
    
    LOG_INFO(QStringLiteral("Double-clicked file: %1 -> target: %2").arg(filePath, targetPath));
    openFileInTab(targetPath);
}

void MainWindow::onTabCloseRequested(int index)
{
    if (index < 0 || index >= m_editorTabs->count()) {
        return;
    }
    
    QWidget *tab = m_editorTabs->widget(index);
    if (auto *fileTab = qobject_cast<FileTab*>(tab)) {
        LOG_INFO(QStringLiteral("Closing tab for file: %1").arg(fileTab->filePath()));
    }
    
    m_editorTabs->removeTab(index);
    tab->deleteLater();
}

void MainWindow::openFileInTab(const QString &filePath)
{
    if (filePath.isEmpty()) {
        LOG_WARNING("Cannot open tab: file path is empty"_L1);
        return;
    }
    
    // Check if tab is already open
    FileTab *existingTab = findTabByFilePath(filePath);
    if (existingTab) {
        // Switch to existing tab
        int tabIndex = m_editorTabs->indexOf(existingTab);
        if (tabIndex >= 0) {
            m_editorTabs->setCurrentIndex(tabIndex);
            LOG_DEBUG(QStringLiteral("Switched to existing tab for file: %1").arg(filePath));
            return;
        }
    }
    
    // Create new tab
    auto *fileTab = new FileTab(filePath, m_chezmoiService.get(), this);
    int tabIndex = m_editorTabs->addTab(fileTab, fileTab->fileName());
    
    // Set tooltip to show full path
    m_editorTabs->setTabToolTip(tabIndex, filePath);
    
    // Switch to the new tab
    m_editorTabs->setCurrentIndex(tabIndex);
    
    LOG_INFO(QStringLiteral("Opened new tab for file: %1").arg(filePath));
}

FileTab* MainWindow::findTabByFilePath(const QString &filePath)
{
    for (int i = 0; i < m_editorTabs->count(); ++i) {
        if (auto *fileTab = qobject_cast<FileTab*>(m_editorTabs->widget(i))) {
            if (fileTab->filePath() == filePath) {
                return fileTab;
            }
        }
    }
    return nullptr;
}

void MainWindow::onFileModified()
{
    // TODO: Handle file modification
    LOG_INFO("File modified"_L1);
}

void MainWindow::loadDotfiles()
{
    LOG_INFO("MainWindow: Loading dotfiles..."_L1);
    
    // Set up the connection between services
    m_dotfileManager->setChezmoiService(m_chezmoiService.get());
    
    // Refresh the file tree
    m_dotfileManager->refreshFiles();
}
