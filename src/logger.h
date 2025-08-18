#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

class Logger : public QObject
{
    Q_OBJECT

public:
    enum LogLevel {
        Debug,
        Info,
        Warning,
        Error
    };

    static Logger* instance();
    
    void setupLogging();
    void log(LogLevel level, const QString &message, const QString &category = QString());
    
    QString getLogFilePath() const;
    QString getLogContents() const;
    void clearLog();

    // Convenience static methods
    static void debug(const QString &message, const QString &category = QString());
    static void info(const QString &message, const QString &category = QString());
    static void warning(const QString &message, const QString &category = QString());
    static void error(const QString &message, const QString &category = QString());

private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();
    
    void writeToFile(const QString &formattedMessage);
    QString formatMessage(LogLevel level, const QString &message, const QString &category);
    QString levelToString(LogLevel level);
    
    static Logger *m_instance;
    QFile *m_logFile;
    QTextStream *m_stream;
    QMutex m_mutex;
    QString m_logFilePath;
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::debug(msg, QString::fromLatin1(__FUNCTION__))
#define LOG_INFO(msg) Logger::info(msg, QString::fromLatin1(__FUNCTION__))
#define LOG_WARNING(msg) Logger::warning(msg, QString::fromLatin1(__FUNCTION__))
#define LOG_ERROR(msg) Logger::error(msg, QString::fromLatin1(__FUNCTION__))

#endif // LOGGER_H
