#include "chezmoiservice.h"

#include <memory>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QTextStream>
#include <QRegularExpression>

ChezmoiService::ChezmoiService(QObject *parent)
    : QObject(parent)
    , m_process(std::make_unique<QProcess>(this))
    , m_chezmoiPath()
    , m_currentOperation()
{
    connect(m_process.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ChezmoiService::onProcessFinished);
    connect(m_process.get(), &QProcess::errorOccurred,
            this, &ChezmoiService::onProcessError);
    
    m_chezmoiPath = getChezmoiExecutable();
}

ChezmoiService::~ChezmoiService() = default;

QString ChezmoiService::getChezmoiExecutable() const
{
    return QStandardPaths::findExecutable(QStringLiteral("chezmoi"));
}

bool ChezmoiService::isChezmoiInitialized() const
{
    QString chezmoiDir = getChezmoiDirectory();
    return QDir(chezmoiDir).exists() && QDir(chezmoiDir + QStringLiteral("/.git")).exists();
}

bool ChezmoiService::initializeRepository(const QString &repositoryUrl)
{
    if (m_chezmoiPath.isEmpty()) {
        Q_EMIT operationCompleted(false, QStringLiteral("chezmoi executable not found"));
        return false;
    }
    
    QStringList args;
    args << QStringLiteral("init");
    
    if (!repositoryUrl.isEmpty()) {
        args << repositoryUrl;
    }
    
    m_currentOperation = QStringLiteral("init");
    return runChezmoiCommand(args, true);
}

QList<ChezmoiService::FileStatus> ChezmoiService::getManagedFiles()
{
    QList<FileStatus> files;
    
    if (!runChezmoiCommand({QStringLiteral("managed"), QStringLiteral("-i")}, false)) {
        return files;
    }
    
    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    QTextStream stream(&output);
    QString line;
    
    while (stream.readLineInto(&line)) {
        if (line.trimmed().isEmpty()) {
            continue;
        }
        
        FileStatus status;
        status.path = line.trimmed();
        status.status = QStringLiteral("managed");
        status.isTemplate = line.contains(QStringLiteral(".tmpl"));
        
        // Get source file info
        QString sourceDir = getChezmoiDirectory();
        QString sourcePath = sourceDir + QStringLiteral("/") + status.path;
        status.sourceFile = QFileInfo(sourcePath);
        
        // Get target file info
        QString targetPath = QDir::homePath() + QStringLiteral("/") + status.path;
        status.targetFile = QFileInfo(targetPath);
        
        files.append(status);
    }
    
    return files;
}

bool ChezmoiService::addFile(const QString &filePath)
{
    if (m_chezmoiPath.isEmpty()) {
        return false;
    }
    
    m_currentOperation = QStringLiteral("add");
    return runChezmoiCommand({QStringLiteral("add"), filePath}, true);
}

bool ChezmoiService::removeFile(const QString &filePath)
{
    if (m_chezmoiPath.isEmpty()) {
        return false;
    }
    
    m_currentOperation = QStringLiteral("remove");
    return runChezmoiCommand({QStringLiteral("remove"), filePath}, true);
}

bool ChezmoiService::applyChanges()
{
    if (m_chezmoiPath.isEmpty()) {
        return false;
    }
    
    m_currentOperation = QStringLiteral("apply");
    return runChezmoiCommand({QStringLiteral("apply")}, true);
}

bool ChezmoiService::updateRepository()
{
    if (m_chezmoiPath.isEmpty()) {
        return false;
    }
    
    m_currentOperation = QStringLiteral("update");
    return runChezmoiCommand({QStringLiteral("update")}, true);
}

QString ChezmoiService::getChezmoiDirectory() const
{
    QProcess process;
    process.start(m_chezmoiPath, {QStringLiteral("source-path")});
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    }
    
    // Fallback to default location
    return QDir::homePath() + QStringLiteral("/.local/share/chezmoi");
}

QString ChezmoiService::getConfigFile() const
{
    QProcess process;
    process.start(m_chezmoiPath, {QStringLiteral("dump-config")});
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        // Parse the config to find the config file path
        // For now, return the default location
    }
    
    return QDir::homePath() + QStringLiteral("/.config/chezmoi/chezmoi.toml");
}

bool ChezmoiService::runChezmoiCommand(const QStringList &arguments, bool async)
{
    if (m_chezmoiPath.isEmpty()) {
        return false;
    }
    
    if (async) {
        m_process->start(m_chezmoiPath, arguments);
        return m_process->waitForStarted();
    } else {
        m_process->start(m_chezmoiPath, arguments);
        return m_process->waitForFinished() && m_process->exitCode() == 0;
    }
}

void ChezmoiService::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    bool success = (exitCode == 0 && exitStatus == QProcess::NormalExit);
    QString message;
    
    if (success) {
        message = QStringLiteral("Operation '%1' completed successfully").arg(m_currentOperation);
    } else {
        QString errorOutput = QString::fromUtf8(m_process->readAllStandardError());
        message = QStringLiteral("Operation '%1' failed: %2").arg(m_currentOperation, errorOutput);
    }
    
    Q_EMIT operationCompleted(success, message);
    m_currentOperation.clear();
}

void ChezmoiService::onProcessError(QProcess::ProcessError error)
{
    QString errorMessage;
    switch (error) {
    case QProcess::FailedToStart:
        errorMessage = QStringLiteral("Failed to start chezmoi process");
        break;
    case QProcess::Crashed:
        errorMessage = QStringLiteral("chezmoi process crashed");
        break;
    case QProcess::Timedout:
        errorMessage = QStringLiteral("chezmoi process timed out");
        break;
    default:
        errorMessage = QStringLiteral("Unknown chezmoi process error");
        break;
    }
    
    Q_EMIT operationCompleted(false, errorMessage);
    m_currentOperation.clear();
}
