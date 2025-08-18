#ifndef DOTFILEMANAGER_H
#define DOTFILEMANAGER_H

#include <QObject>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <memory>

class ChezmoiService;

class DotfileManager : public QAbstractItemModel
{
    Q_OBJECT

public:
    struct DotfileItem {
        QString name;
        QString fullPath;
        QString status;
        bool isDirectory;
        bool isTemplate;
        QList<DotfileItem*> children;
        DotfileItem *parent;
        
        DotfileItem(const QString &n = QString(), DotfileItem *p = nullptr)
            : name(n), isDirectory(false), isTemplate(false), parent(p) {}
        
        ~DotfileItem() {
            qDeleteAll(children);
        }
    };

    explicit DotfileManager(QObject *parent = nullptr);
    ~DotfileManager() override;

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setChezmoiService(ChezmoiService *service);
    void refreshFiles();
    QString getFilePath(const QModelIndex &index) const;
    bool isTemplate(const QModelIndex &index) const;

Q_SIGNALS:
    void fileModified(const QString &filePath);
    void filesRefreshed();

private:
    void buildFileTree();
    DotfileItem *getItem(const QModelIndex &index) const;
    void addFileToTree(const QString &relativePath, const QString &fullPath, const QString &status, bool isTemplate);
    DotfileItem *findOrCreateParent(const QString &path, DotfileItem *root);

    ChezmoiService *m_chezmoiService;
    std::unique_ptr<DotfileItem> m_rootItem;
};


#endif // DOTFILEMANAGER_H
