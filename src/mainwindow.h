#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KXmlGuiWindow>
#include <QStackedWidget>
#include <memory>

class QTreeView;
class QTextEdit;
class QTabWidget;
class QLabel;
class ChezmoiService;
class DotfileManager;
class ConfigEditor;

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private Q_SLOTS:
    void openSettings();
    void refreshFiles();
    void syncFiles();
    void showAbout();
    void showLogViewer();
    void toggleSidebar();
    void onFileSelected(const QString &filePath);
    void onFileModified();

private:
    void setupUI();
    void setupActions();
    void setupStatusBar();
    void updateGitStatus();
    void loadDotfiles();

    QTreeView *m_fileTreeView;
    QTabWidget *m_editorTabs;
    
    // Status bar widgets
    QLabel *m_statusLeft;
    QLabel *m_statusCenter;
    QLabel *m_statusRight;
    
    std::unique_ptr<ChezmoiService> m_chezmoiService;
    std::unique_ptr<DotfileManager> m_dotfileManager;
    std::unique_ptr<ConfigEditor> m_configEditor;
    
    QString m_currentFile;
};

#endif // MAINWINDOW_H
