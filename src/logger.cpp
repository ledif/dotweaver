#include "logger.h"
#include <memory>
#include <QApplication>
#include <QDebug>

using namespace Qt::Literals::StringLiterals;

Logger* Logger::m_instance = nullptr;

Logger* Logger::instance()
{
    if (!m_instance) {
        m_instance = new Logger();
    }
    return m_instance;
}

Logger::Logger(QObject *parent)
    : QObject(parent)
    , m_logFile(nullptr)
    , m_stream(nullptr)
{
    setupLogging();
}

Logger::~Logger()
{
    if (m_stream) {
        m_stream.reset();
    }
    if (m_logFile && m_logFile->isOpen()) {
        m_logFile->close();
        m_logFile.reset();
    }
}

void Logger::setupLogging()
{
    // Determine log directory - works both in and out of Flatpak
    QString logDir;
    
    // Try XDG_DATA_HOME first (Flatpak sets this)
    QString xdgDataHome = QString::fromLocal8Bit(qgetenv("XDG_DATA_HOME"));
    if (!xdgDataHome.isEmpty()) {
        logDir = xdgDataHome + "/dotweaver"_L1;
    } else {
        // Fall back to standard location
        logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    }
    
    // Create directory if it doesn't exist
    QDir().mkpath(logDir);
    
    m_logFilePath = logDir + "/dotweaver.log"_L1;
    
    // Open log file
    m_logFile = std::make_unique<QFile>(m_logFilePath);
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_stream = std::make_unique<QTextStream>(m_logFile.get());
        m_stream->setEncoding(QStringConverter::Utf8);
        
        // Log startup
        log(Info, "DotWeaver started"_L1, "Application"_L1);
    } else {
        qWarning() << "Could not open log file:" << m_logFilePath;
    }
}

void Logger::log(LogLevel level, const QString &message, const QString &category)
{
    QMutexLocker locker(&m_mutex);
    
    QString formattedMessage = formatMessage(level, message, category);
    
    // Write to file
    writeToFile(formattedMessage);
    
    // Also output to console for development
    switch (level) {
    case Debug:
        qDebug().noquote() << formattedMessage;
        break;
    case Info:
        qInfo().noquote() << formattedMessage;
        break;
    case Warning:
        qWarning().noquote() << formattedMessage;
        break;
    case Error:
        qCritical().noquote() << formattedMessage;
        break;
    }
}

void Logger::writeToFile(const QString &formattedMessage)
{
    if (m_stream) {
        *m_stream << formattedMessage << Qt::endl;
        m_stream->flush();
    }
}

QString Logger::formatMessage(LogLevel level, const QString &message, const QString &category)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"_L1);
    QString levelStr = levelToString(level);
    
    if (category.isEmpty()) {
        return "[%1] %2: %3"_L1.arg(timestamp, levelStr, message);
    } else {
        return "[%1] %2 [%3]: %4"_L1.arg(timestamp, levelStr, category, message);
    }
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
    case Debug: return "DEBUG"_L1;
    case Info: return "INFO"_L1;
    case Warning: return "WARN"_L1;
    case Error: return "ERROR"_L1;
    default: return "UNKNOWN"_L1;
    }
}

QString Logger::getLogFilePath() const
{
    return m_logFilePath;
}

QString Logger::getLogContents() const
{
    QFile file(m_logFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        return stream.readAll();
    }
    return "Could not read log file: %1"_L1.arg(m_logFilePath);
}

void Logger::clearLog()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_logFile && m_logFile->isOpen()) {
        m_logFile->resize(0);
        log(Info, "Log file cleared"_L1, "Logger"_L1);
    }
}

void Logger::debug(const QString &message, const QString &category)
{
    instance()->log(Debug, message, category);
}

void Logger::info(const QString &message, const QString &category)
{
    instance()->log(Info, message, category);
}

void Logger::warning(const QString &message, const QString &category)
{
    instance()->log(Warning, message, category);
}

void Logger::error(const QString &message, const QString &category)
{
    instance()->log(Error, message, category);
}
