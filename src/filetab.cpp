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
#include <QMimeDatabase>
#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QSizePolicy>
#include <QWidget>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <KTextEditor/Editor>
#include <KTextEditor/Cursor>
#include <KLocalizedString>
#include <QIcon>

using namespace Qt::Literals::StringLiterals;

FileTab::FileTab(const QString &filePath, ChezmoiService *chezmoiService, QWidget *parent)
    : QWidget(parent)
    , m_filePath(filePath)
    , m_chezmoiService(chezmoiService)
    , m_document(nullptr)
    , m_textView(nullptr)
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
    
    // Create KTextEditor document and view
    auto editor = KTextEditor::Editor::instance();
    if (!editor) {
        LOG_ERROR("Failed to get KTextEditor instance"_L1);
        return;
    }
    
    m_document = editor->createDocument(this);
    if (!m_document) {
        LOG_ERROR("Failed to create KTextEditor document"_L1);
        return;
    }
    
    m_textView = m_document->createView(this);
    if (!m_textView) {
        LOG_ERROR("Failed to create KTextEditor view"_L1);
        return;
    }
    
    // Configure the view
    m_document->setReadWrite(false); // Make it read-only
    
    m_mainLayout->addWidget(m_textView);
    
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
    if (m_filePath.isEmpty() || !m_document) {
        LOG_WARNING("Cannot load file content: invalid file path or document"_L1);
        return;
    }
    
    // Try to use KTextEditor's built-in file loading
    QUrl fileUrl = QUrl::fromLocalFile(m_filePath);
    LOG_INFO(QStringLiteral("Attempting to open file: %1").arg(fileUrl.toString()));
    
    // Check if file exists first
    QFileInfo fileInfo(m_filePath);
    if (!fileInfo.exists()) {
        LOG_WARNING(QStringLiteral("File does not exist: %1").arg(m_filePath));
        m_document->setText(i18n("Error: File does not exist\n%1").arg(m_filePath));
        return;
    }
    
    if (!fileInfo.isReadable()) {
        LOG_WARNING(QStringLiteral("File is not readable: %1").arg(m_filePath));
        m_document->setText(i18n("Error: File is not readable\n%1").arg(m_filePath));
        return;
    }
    
    // Try to open the file using KTextEditor's openUrl method
    if (m_document->openUrl(fileUrl)) {
        LOG_INFO(QStringLiteral("Successfully opened file via KTextEditor: %1").arg(m_filePath));
        // Make sure it's read-only after loading
        m_document->setReadWrite(false);
    } else {
        LOG_WARNING(QStringLiteral("Failed to open file via KTextEditor, trying manual load: %1").arg(m_filePath));
        
        // Fallback: manual file reading
        QFile file(m_filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            LOG_INFO(QStringLiteral("Manually loaded file content: %1 (%2 chars)").arg(m_filePath).arg(content.length()));
            m_document->setText(content);
        } else {
            QString errorMsg = i18n("Error: Unable to read file\n%1\n%2").arg(m_filePath, file.errorString());
            LOG_WARNING(QStringLiteral("Failed to read file: %1 - %2").arg(m_filePath, file.errorString()));
            m_document->setText(errorMsg);
        }
    }
    
    // Move cursor to the beginning
    if (m_textView) {
        m_textView->setCursorPosition(KTextEditor::Cursor(0, 0));
    }
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
