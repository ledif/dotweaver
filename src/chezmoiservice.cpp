#include "chezmoiservice.h"
#include "logger.h"

#include <memory>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QTextStream>
#include <QRegularExpression>

using namespace Qt::Literals::StringLiterals;

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
    
    LOG_INFO(QStringLiteral("ChezmoiService initialized with path: %1").arg(m_chezmoiPath.isEmpty() ? "NOT FOUND"_L1 : m_chezmoiPath));
}

ChezmoiService::~ChezmoiService() = default;

QString ChezmoiService::getChezmoiExecutable() const
{
    QString path = QStandardPaths::findExecutable(QStringLiteral("chezmoi"));
    LOG_INFO(QStringLiteral("Looking for chezmoi executable: %1").arg(path.isEmpty() ? "NOT FOUND"_L1 : path));
    return path;
}

bool ChezmoiService::isChezmoiInitialized() const
{
    QString chezmoiDir = getChezmoiDirectory();
    bool dirExists = QDir(chezmoiDir).exists();
    bool gitExists = QDir(chezmoiDir + QStringLiteral("/.git")).exists();
    bool initialized = dirExists && gitExists;
    
    LOG_INFO(QStringLiteral("Checking chezmoi initialization:"));
    LOG_INFO(QStringLiteral("  Directory: %1 (exists: %2)").arg(chezmoiDir).arg(dirExists ? "yes"_L1 : "no"_L1));
    LOG_INFO(QStringLiteral("  Git repo: %1/.git (exists: %2)").arg(chezmoiDir).arg(gitExists ? "yes"_L1 : "no"_L1));
    LOG_INFO(QStringLiteral("  Initialized: %1").arg(initialized ? "yes"_L1 : "no"_L1));
    
    return initialized;
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
    
    LOG_INFO("Getting managed files from chezmoi"_L1);
    
    if (m_chezmoiPath.isEmpty()) {
        LOG_ERROR("Cannot get managed files: chezmoi executable not found"_L1);
        return files;
    }

    // First get the file statuses
    QHash<QString, QString> fileStatuses = getFileStatuses();

    if (!runChezmoiCommand({QStringLiteral("managed"), QStringLiteral("--exclude=dirs")}, false)) {
        LOG_ERROR("Failed to run 'chezmoi managed --exclude=dirs' command"_L1);
        return files;
    }

    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    LOG_INFO(QStringLiteral("Chezmoi managed command output (%1 chars):").arg(output.length()));
    LOG_INFO(output.isEmpty() ? "  (empty output)"_L1 : QStringLiteral("  %1").arg(output.left(200) + (output.length() > 200 ? "..."_L1 : ""_L1)));

    QTextStream stream(&output);
    QString line;
    int fileCount = 0;

    // Cache the source directory to avoid multiple calls
    QString sourceDir = getChezmoiDirectory();

    while (stream.readLineInto(&line)) {
        if (line.trimmed().isEmpty()) {
            continue;
        }

        fileCount++;
        LOG_DEBUG(QStringLiteral("Processing managed file: %1").arg(line.trimmed()));

        FileStatus status;
        status.path = line.trimmed();
        
        // Use actual status from chezmoi status, or default to "managed"
        status.status = fileStatuses.value(status.path, u"managed"_s);
        status.isTemplate = line.contains(u".tmpl"_s);

        // Get source file info
        QString sourcePath = sourceDir + QStringLiteral("/") + status.path;
        status.sourceFile = QFileInfo(sourcePath);

        // Get target file info
        QString targetPath = QDir::homePath() + QStringLiteral("/") + status.path;
        status.targetFile = QFileInfo(targetPath);

        files.append(status);
    }
    
    LOG_INFO(QStringLiteral("Found %1 managed files").arg(fileCount));
    return files;
}

QHash<QString, QString> ChezmoiService::getFileStatuses()
{
    QHash<QString, QString> statuses;
    
    LOG_INFO("Getting file statuses from 'chezmoi status'"_L1);
    
    if (m_chezmoiPath.isEmpty()) {
        LOG_ERROR("Cannot get file statuses: chezmoi executable not found"_L1);
        return statuses;
    }
    
    if (!runChezmoiCommand({QStringLiteral("status")}, false)) {
        LOG_ERROR("Failed to run 'chezmoi status' command"_L1);
        return statuses;
    }
    
    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    LOG_INFO(QStringLiteral("Chezmoi status command output (%1 chars):").arg(output.length()));
    
    QTextStream stream(&output);
    QString line;
    
    while (stream.readLineInto(&line)) {
        if (line.trimmed().isEmpty()) {
            continue;
        }
        
        // Parse chezmoi status format: "MM filename"
        // First character: difference between last written state and actual state
        // Second character: difference between actual state and target state
        if (line.length() >= 3) {
            QString statusChars = line.left(2);
            QString filePath = line.mid(3).trimmed();
            
            // Convert status characters to a descriptive status
            QString status = u"unchanged"_s;
            if (statusChars.contains(u'M')) {
                status = u"modified"_s;
            } else if (statusChars.contains(u'A')) {
                status = u"added"_s;
            } else if (statusChars.contains(u'D')) {
                status = u"deleted"_s;
            } else if (statusChars.contains(u'R')) {
                status = u"script"_s;
            }
            
            statuses[filePath] = status;
            LOG_DEBUG(QStringLiteral("File status: %1 -> %2").arg(filePath, status));
        }
    }
    
    LOG_INFO(QStringLiteral("Found %1 files with status changes").arg(statuses.size()));
    return statuses;
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
    if (m_chezmoiPath.isEmpty()) {
        QString fallback = QDir::homePath() + QStringLiteral("/.local/share/chezmoi");
        LOG_WARNING(QStringLiteral("Chezmoi executable not found, using fallback directory: %1").arg(fallback));
        return fallback;
    }
    
    QProcess process;
    LOG_DEBUG("Running 'chezmoi source-path' to get source directory"_L1);
    process.start(m_chezmoiPath, {QStringLiteral("source-path")});
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        QString result = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        LOG_INFO(QStringLiteral("Chezmoi source directory: %1").arg(result));
        return result;
    } else {
        QString error = QString::fromUtf8(process.readAllStandardError());
        QString fallback = QDir::homePath() + QStringLiteral("/.local/share/chezmoi");
        LOG_WARNING(QStringLiteral("Failed to get chezmoi source directory (exit code: %1, error: %2), using fallback: %3")
                   .arg(process.exitCode()).arg(error).arg(fallback));
        return fallback;
    }
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
        LOG_ERROR("Cannot run chezmoi command: executable not found"_L1);
        return false;
    }
    
    QString cmdLine = QStringLiteral("%1 %2").arg(m_chezmoiPath, arguments.join(QChar(u' ')));
    LOG_DEBUG(QStringLiteral("Running chezmoi command: %1 (async: %2)").arg(cmdLine).arg(async ? "yes"_L1 : "no"_L1));
    
    if (async) {
        m_process->start(m_chezmoiPath, arguments);
        bool started = m_process->waitForStarted();
        LOG_DEBUG(QStringLiteral("Command started: %1").arg(started ? "yes"_L1 : "no"_L1));
        return started;
    } else {
        m_process->start(m_chezmoiPath, arguments);
        bool finished = m_process->waitForFinished();
        int exitCode = m_process->exitCode();
        bool success = finished && exitCode == 0;
        
        if (!finished) {
            LOG_ERROR("Command did not finish properly"_L1);
        } else if (exitCode != 0) {
            QString errorOutput = QString::fromUtf8(m_process->readAllStandardError());
            LOG_ERROR(QStringLiteral("Command failed with exit code %1, error: %2").arg(exitCode).arg(errorOutput));
        } else {
            LOG_DEBUG("Command completed successfully"_L1);
        }
        
        return success;
    }
}

void ChezmoiService::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    // Only emit operationCompleted for async operations (when m_currentOperation is set)
    if (m_currentOperation.isEmpty()) {
        return;
    }
    
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

QString ChezmoiService::getCatFileContent(const QString &filePath)
{
    if (m_chezmoiPath.isEmpty()) {
        LOG_ERROR("Cannot get file content: chezmoi executable not found"_L1);
        return QString();
    }
    
    LOG_DEBUG(QStringLiteral("Getting file content via chezmoi cat: %1").arg(filePath));
    
    if (!runChezmoiCommand({QStringLiteral("cat"), filePath}, false)) {
        LOG_WARNING(QStringLiteral("Failed to run 'chezmoi cat' for file: %1").arg(filePath));
        return QString();
    }
    
    QString content = QString::fromUtf8(m_process->readAllStandardOutput());
    LOG_DEBUG(QStringLiteral("Retrieved content (%1 chars) for file: %2").arg(content.length()).arg(filePath));
    
    return content;
}

QString ChezmoiService::getSourcePath(const QString &filePath)
{
    if (m_chezmoiPath.isEmpty()) {
        LOG_ERROR("Cannot get source path: chezmoi executable not found"_L1);
        return QString();
    }
    
    LOG_DEBUG(QStringLiteral("Getting source path for file: %1").arg(filePath));
    
    if (!runChezmoiCommand({QStringLiteral("source-path"), filePath}, false)) {
        LOG_WARNING(QStringLiteral("Failed to run 'chezmoi source-path' for file: %1").arg(filePath));
        return QString();
    }
    
    QString sourcePath = QString::fromUtf8(m_process->readAllStandardOutput()).trimmed();
    LOG_DEBUG(QStringLiteral("Source path for %1: %2").arg(filePath, sourcePath));
    
    return sourcePath;
}

QString ChezmoiService::getDestinationDirectory() const
{
    if (m_chezmoiPath.isEmpty()) {
        LOG_ERROR("Cannot get destination directory: chezmoi executable not found"_L1);
        return QString();
    }
    
    LOG_DEBUG("Getting destination directory from chezmoi config"_L1);
    
    // Use a temporary process since we need this to be synchronous and not interfere with m_process
    QProcess tempProcess;
    tempProcess.start(m_chezmoiPath, {QStringLiteral("dump-config"), QStringLiteral("--format=json")});
    
    if (!tempProcess.waitForFinished() || tempProcess.exitCode() != 0) {
        LOG_WARNING("Failed to get chezmoi config, falling back to home directory"_L1);
        return QDir::homePath();
    }
    
    QString output = QString::fromUtf8(tempProcess.readAllStandardOutput());
    
    // Parse the JSON to extract destDir
    // Simple parsing since we only need destDir
    QRegularExpression destDirRegex(QStringLiteral("\"destDir\"\\s*:\\s*\"([^\"]+)\""));
    QRegularExpressionMatch match = destDirRegex.match(output);
    
    if (match.hasMatch()) {
        QString destDir = match.captured(1);
        LOG_DEBUG(QStringLiteral("Found destination directory: %1").arg(destDir));
        return destDir;
    }
    
    LOG_WARNING("Could not parse destDir from chezmoi config, falling back to home directory"_L1);
    return QDir::homePath();
}

QString ChezmoiService::convertToTargetPath(const QString &sourcePath) const
{
    if (sourcePath.isEmpty()) {
        return QString();
    }
    
    QString sourceDir = getChezmoiDirectory();
    QString destDir = getDestinationDirectory();
    
    if (sourceDir.isEmpty() || destDir.isEmpty()) {
        LOG_WARNING("Cannot convert path: missing source or destination directory"_L1);
        return sourcePath;
    }
    
    // Check if the path is actually a source path
    if (!sourcePath.startsWith(sourceDir)) {
        // It might already be a target path or a relative path
        LOG_DEBUG(QStringLiteral("Path doesn't start with source directory, assuming it's already a target path: %1").arg(sourcePath));
        return sourcePath;
    }
    
    // Remove the source directory prefix and the leading slash
    QString relativePath = sourcePath.mid(sourceDir.length());
    if (relativePath.startsWith(u'/')) {
        relativePath = relativePath.mid(1);
    }
    
    // Remove chezmoi prefixes from the filename
    // This is a simplified conversion - chezmoi has complex rules for filename mapping
    QStringList pathComponents = relativePath.split(u'/');
    for (QString &component : pathComponents) {
        // Remove common chezmoi prefixes
        if (component.startsWith(QStringLiteral("dot_"))) {
            component = u'.' + component.mid(4);
        } else if (component.startsWith(QStringLiteral("private_"))) {
            component = component.mid(8);
        } else if (component.startsWith(QStringLiteral("executable_"))) {
            component = component.mid(11);
        }
        // Remove .tmpl suffix for templates
        if (component.endsWith(QStringLiteral(".tmpl"))) {
            component = component.left(component.length() - 5);
        }
    }
    
    QString targetPath = destDir + u'/' + pathComponents.join(u'/');
    LOG_DEBUG(QStringLiteral("Converted source path %1 to target path %2").arg(sourcePath, targetPath));
    
    return targetPath;
}
