#include "logviewer.h"
#include "logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QTextStream>
#include <QFile>

#include <KLocalizedString>

using namespace Qt::Literals::StringLiterals;

LogViewer::LogViewer(QWidget *parent)
    : QDialog(parent)
    , m_logTextEdit(nullptr)
    , m_refreshButton(nullptr)
    , m_clearButton(nullptr)
    , m_saveButton(nullptr)
    , m_closeButton(nullptr)
{
    setupUI();
    // Call refreshLog after UI is set up
    QMetaObject::invokeMethod(this, &LogViewer::refreshLog, Qt::QueuedConnection);
}

void LogViewer::setupUI()
{
    setWindowTitle(i18n("DotWeaver Log Viewer"));
    setModal(false);
    resize(800, 600);
    
    auto *layout = new QVBoxLayout(this);
    
    // Log text area
    m_logTextEdit = new QTextEdit(this);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setFont(QFont(QStringLiteral("monospace")));
    m_logTextEdit->setLineWrapMode(QTextEdit::NoWrap);
    layout->addWidget(m_logTextEdit);
    
    // Button layout
    auto *buttonLayout = new QHBoxLayout();
    
    m_refreshButton = new QPushButton(i18n("&Refresh"), this);
    m_refreshButton->setToolTip(i18n("Reload log contents from file"));
    connect(m_refreshButton, &QPushButton::clicked, this, &LogViewer::refreshLog);
    buttonLayout->addWidget(m_refreshButton);
    
    m_clearButton = new QPushButton(i18n("&Clear Log"), this);
    m_clearButton->setToolTip(i18n("Clear all log entries"));
    connect(m_clearButton, &QPushButton::clicked, this, &LogViewer::clearLog);
    buttonLayout->addWidget(m_clearButton);
    
    m_saveButton = new QPushButton(i18n("&Save As..."), this);
    m_saveButton->setToolTip(i18n("Save log to a file"));
    connect(m_saveButton, &QPushButton::clicked, this, &LogViewer::saveLog);
    buttonLayout->addWidget(m_saveButton);
    
    buttonLayout->addStretch();
    
    m_closeButton = new QPushButton(i18n("&Close"), this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);
    
    layout->addLayout(buttonLayout);
    
    // Set focus to refresh button
    m_refreshButton->setDefault(true);
}

void LogViewer::refreshLog()
{
    QString logContents = Logger::instance()->getLogContents();
    m_logTextEdit->setPlainText(logContents);
    
    // Scroll to bottom to show latest entries
    QTextCursor cursor = m_logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logTextEdit->setTextCursor(cursor);
    
    Logger::debug("Log viewer refreshed"_L1, "LogViewer"_L1);
}

void LogViewer::clearLog()
{
    int result = QMessageBox::question(this,
        i18n("Clear Log"),
        i18n("Are you sure you want to clear all log entries? This action cannot be undone."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        Logger::instance()->clearLog();
        refreshLog();
        QMessageBox::information(this, i18n("Log Cleared"), i18n("The log has been cleared successfully."));
    }
}

void LogViewer::saveLog()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        i18n("Save Log File"),
        QStringLiteral("dotweaver-log-%1.txt").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd-hh-mm-ss"))),
        i18n("Text Files (*.txt);;All Files (*)"));
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        stream << m_logTextEdit->toPlainText();
        
        QMessageBox::information(this, i18n("Log Saved"), 
            i18n("Log saved successfully to:\n%1").arg(fileName));
        
        Logger::info(QStringLiteral("Log saved to: %1").arg(fileName), QStringLiteral("LogViewer"));
    } else {
        QMessageBox::critical(this, i18n("Save Error"), 
            i18n("Could not save log file:\n%1").arg(fileName));
        
        Logger::error(QStringLiteral("Failed to save log to: %1").arg(fileName), QStringLiteral("LogViewer"));
    }
}
