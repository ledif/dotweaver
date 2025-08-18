#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KXmlGuiWindow>
#include <QStackedWidget>

class QSplitter;
class QTreeView;
class QTextEdit;
class QListWidget;
class QTabWidget;
class ChezmoiService;
class DotfileManager;
class ConfigEditor;

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void openSettings();
    void refreshFiles();
    void syncFiles();
    void showAbout();
    void onFileSelected(const QString &filePath);
    void onFileModified();

private:
    void setupUI();
    void setupActions();
    void setupStatusBar();
    void loadDotfiles();

    QSplitter *m_mainSplitter;
    QTreeView *m_fileTreeView;
    QTabWidget *m_editorTabs;
    QListWidget *m_statusList;
    
    ChezmoiService *m_chezmoiService;
    DotfileManager *m_dotfileManager;
    ConfigEditor *m_configEditor;
    
    QString m_currentFile;
};

#endif // MAINWINDOW_H
