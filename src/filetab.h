#ifndef FILETAB_H
#define FILETAB_H

#include <QWidget>
#include <QString>
#include <memory>

class QPushButton;
class QVBoxLayout;
class QToolBar;
class ChezmoiService;

namespace KTextEditor {
    class Document;
    class View;
}

/**
 * @brief Widget representing a single file tab with read-only content and external editor option
 */
class FileTab : public QWidget
{
    Q_OBJECT

public:
    explicit FileTab(const QString &filePath, ChezmoiService *chezmoiService, QWidget *parent = nullptr);
    ~FileTab() override = default;

    const QString &filePath() const { return m_filePath; }
    const QString &fileName() const { return m_fileName; }

public Q_SLOTS:
    void refreshContent();

private Q_SLOTS:
    void openInExternalEditor();

private:
    void setupUI();
    void loadFileContent();
    QString determineFileExtension() const;

    QString m_filePath;
    QString m_fileName;
    ChezmoiService *m_chezmoiService;
    
    KTextEditor::Document *m_document;
    KTextEditor::View *m_textView;
    QPushButton *m_openExternalButton;
    QVBoxLayout *m_mainLayout;
    QToolBar *m_bottomToolBar;
};

#endif // FILETAB_H
