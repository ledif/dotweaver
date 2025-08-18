#ifndef CHEZMOISERVICE_H
#define CHEZMOISERVICE_H

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QFileInfo>
#include <QHash>
#include <memory>

class ChezmoiService : public QObject
{
    Q_OBJECT

public:
    explicit ChezmoiService(QObject *parent = nullptr);
    ~ChezmoiService() override;

    struct FileStatus {
        QString path;
        QString status; // "managed", "unmanaged", "modified", etc.
        bool isTemplate;
        QFileInfo sourceFile;
        QFileInfo targetFile;
    };

    bool isChezmoiInitialized() const;
    bool initializeRepository(const QString &repositoryUrl = QString());
    QList<FileStatus> getManagedFiles();
    QHash<QString, QString> getFileStatuses();
    bool addFile(const QString &filePath);
    bool removeFile(const QString &filePath);
    bool applyChanges();
    bool updateRepository();
    QString getChezmoiDirectory() const;
    QString getConfigFile() const;

Q_SIGNALS:
    void operationCompleted(bool success, const QString &message);
    void fileStatusChanged(const QString &filePath, const QString &status);
    void progressUpdated(int percentage);

private Q_SLOTS:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    bool runChezmoiCommand(const QStringList &arguments, bool async = false);
    QString getChezmoiExecutable() const;
    void parseStatusOutput(const QString &output);

    std::unique_ptr<QProcess> m_process;
    QString m_chezmoiPath;
    QString m_currentOperation;
};

#endif // CHEZMOISERVICE_H
