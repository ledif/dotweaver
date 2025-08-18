#include "filetab.h"
#include "chezmoiservice.h"
#include "logger.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QToolBar>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QSizePolicy>
#include <QWidget>
#include <KTextEdit>
#include <KLocalizedString>
#include <QIcon>

using namespace Qt::Literals::StringLiterals;

FileTab::FileTab(const QString &filePath, ChezmoiService *chezmoiService, QWidget *parent)
    : QWidget(parent)
    , m_filePath(filePath)
    , m_chezmoiService(chezmoiService)
    , m_textEdit(nullptr)
    , m_openExternalButton(nullptr)
    , m_mainLayout(nullptr)
    , m_bottomToolBar(nullptr)
{
    // Extract just the filename for display
    QFileInfo fileInfo(filePath);
    m_fileName = fileInfo.fileName();
    
    setupUI();
    loadFileContent();
}

void FileTab::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Create the KTextEdit for file content
    m_textEdit = new KTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setLineWrapMode(QTextEdit::NoWrap);
    
    // Use a monospace font for code files
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_textEdit->setFont(monoFont);
    
    m_mainLayout->addWidget(m_textEdit);
    
    // Create bottom toolbar
    m_bottomToolBar = new QToolBar(this);
    m_bottomToolBar->setMovable(false);
    m_bottomToolBar->setFloatable(false);
    m_bottomToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    
    m_openExternalButton = new QPushButton(this);
    m_openExternalButton->setText(i18n("Open in External Editor"));
    m_openExternalButton->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    m_openExternalButton->setToolTip(i18n("Open this file in your system's default text editor"));
    
    connect(m_openExternalButton, &QPushButton::clicked, 
            this, &FileTab::openInExternalEditor);
    
    // Add button to toolbar
    m_bottomToolBar->addWidget(m_openExternalButton);
    
    // Add stretch to push button to the left
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_bottomToolBar->addWidget(spacer);
    
    m_mainLayout->addWidget(m_bottomToolBar);
}

void FileTab::loadFileContent()
{
    if (m_filePath.isEmpty()) {
        m_textEdit->setPlainText(i18n("Error: No file path specified"));
        return;
    }
    
    QString content;
    
    // Try to read the target file directly first
    QFile file(m_filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        content = in.readAll();
        LOG_INFO(QStringLiteral("Loaded target file content directly: %1").arg(m_filePath));
    } else {
        // Fallback: try to get content via chezmoi cat if it's a managed file
        if (m_chezmoiService) {
            content = m_chezmoiService->getCatFileContent(m_filePath);
            if (!content.isEmpty()) {
                LOG_INFO(QStringLiteral("Loaded file content via chezmoi cat: %1").arg(m_filePath));
            }
        }
        
        // If still empty, show error
        if (content.isEmpty()) {
            content = i18n("Error: Unable to read file\n%1").arg(file.errorString());
            LOG_WARNING(QStringLiteral("Failed to load file: %1 - %2").arg(m_filePath, file.errorString()));
        }
    }
    
    m_textEdit->setPlainText(content);
    
    // Move cursor to the beginning
    QTextCursor cursor = m_textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start);
    m_textEdit->setTextCursor(cursor);
}

void FileTab::refreshContent()
{
    loadFileContent();
    LOG_INFO(QStringLiteral("Refreshed content for file tab: %1").arg(m_filePath));
}

void FileTab::openInExternalEditor()
{
    if (m_filePath.isEmpty()) {
        LOG_WARNING("Cannot open external editor: file path is empty"_L1);
        return;
    }
    
    // For editing, we want to open the source file (the one chezmoi manages)
    QString pathToEdit = m_filePath;
    if (m_chezmoiService) {
        QString sourcePath = m_chezmoiService->getSourcePath(m_filePath);
        if (!sourcePath.isEmpty()) {
            pathToEdit = sourcePath;
            LOG_INFO(QStringLiteral("Will edit source file: %1 (for target: %2)").arg(sourcePath, m_filePath));
        } else {
            LOG_INFO(QStringLiteral("No source path found, will edit target file directly: %1").arg(m_filePath));
        }
    }
    
    LOG_INFO(QStringLiteral("Opening file in external editor: %1").arg(pathToEdit));
    
    // Try to open with the system's default text editor
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(pathToEdit))) {
        // Fallback: try common text editors
        QStringList editors = {
            QStringLiteral("kate"),
            QStringLiteral("gedit"),
            QStringLiteral("nano"),
            QStringLiteral("vim"),
            QStringLiteral("emacs"),
            QStringLiteral("code"),       // VS Code
            QStringLiteral("codium"),     // VS Codium
            QStringLiteral("atom")
        };
        
        bool opened = false;
        for (const QString &editor : editors) {
            QProcess process;
            process.setProgram(editor);
            process.setArguments({pathToEdit});
            
            if (process.startDetached()) {
                opened = true;
                LOG_INFO(QStringLiteral("Opened file with %1: %2").arg(editor, pathToEdit));
                break;
            }
        }
        
        if (!opened) {
            QMessageBox::warning(this, 
                                i18n("Unable to Open File"),
                                i18n("Could not find a suitable text editor to open the file.\n\n"
                                     "File path: %1").arg(pathToEdit));
            LOG_WARNING(QStringLiteral("Failed to open file in any external editor: %1").arg(pathToEdit));
        }
    }
}

QString FileTab::determineFileExtension() const
{
    QFileInfo fileInfo(m_filePath);
    return fileInfo.suffix().toLower();
}
