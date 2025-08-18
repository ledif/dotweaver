#include "dotfilemanager.h"
#include "chezmoiservice.h"

#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QDebug>
#include <QStringList>

DotfileManager::DotfileManager(QObject *parent)
    : QAbstractItemModel(parent)
    , m_chezmoiService(nullptr)
    , m_fileWatcher(new QFileSystemWatcher(this))
    , m_rootItem(new DotfileItem())
{
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &DotfileManager::onFileChanged);
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged,
            this, &DotfileManager::onDirectoryChanged);
}

DotfileManager::~DotfileManager()
{
    delete m_rootItem;
}

void DotfileManager::setChezmoiService(ChezmoiService *service)
{
    m_chezmoiService = service;
    refreshFiles();
}

void DotfileManager::refreshFiles()
{
    if (!m_chezmoiService) {
        return;
    }
    
    beginResetModel();
    
    // Clear existing data
    delete m_rootItem;
    m_rootItem = new DotfileItem();
    
    // Clear file watcher
    if (!m_fileWatcher->files().isEmpty()) {
        m_fileWatcher->removePaths(m_fileWatcher->files());
    }
    if (!m_fileWatcher->directories().isEmpty()) {
        m_fileWatcher->removePaths(m_fileWatcher->directories());
    }
    
    buildFileTree();
    
    endResetModel();
    Q_EMIT filesRefreshed();
}

void DotfileManager::buildFileTree()
{
    if (!m_chezmoiService) {
        return;
    }
    
    auto files = m_chezmoiService->getManagedFiles();
    
    for (const auto &file : files) {
        addFileToTree(file.path, file.sourceFile.absoluteFilePath(), file.status, file.isTemplate);
        
        // Watch the source file for changes
        if (file.sourceFile.exists()) {
            m_fileWatcher->addPath(file.sourceFile.absoluteFilePath());
        }
    }
    
    // Also watch the chezmoi source directory
    QString sourceDir = m_chezmoiService->getChezmoiDirectory();
    if (QDir(sourceDir).exists()) {
        m_fileWatcher->addPath(sourceDir);
    }
}

void DotfileManager::addFileToTree(const QString &relativePath, const QString &fullPath, const QString &status, bool isTemplate)
{
    QStringList pathParts = relativePath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        return;
    }
    
    DotfileItem *currentParent = m_rootItem;
    
    // Navigate/create the directory structure
    for (int i = 0; i < pathParts.size() - 1; ++i) {
        currentParent = findOrCreateParent(pathParts[i], currentParent);
    }
    
    // Add the file
    auto *fileItem = new DotfileItem(pathParts.last(), currentParent);
    fileItem->fullPath = fullPath;
    fileItem->status = status;
    fileItem->isTemplate = isTemplate;
    fileItem->isDirectory = QFileInfo(fullPath).isDir();
    
    currentParent->children.append(fileItem);
}

DotfileManager::DotfileItem *DotfileManager::findOrCreateParent(const QString &name, DotfileItem *parent)
{
    // Look for existing child with this name
    for (auto *child : parent->children) {
        if (child->name == name && child->isDirectory) {
            return child;
        }
    }
    
    // Create new directory item
    auto *dirItem = new DotfileItem(name, parent);
    dirItem->isDirectory = true;
    parent->children.append(dirItem);
    
    return dirItem;
}

QModelIndex DotfileManager::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }
    
    DotfileItem *parentItem = getItem(parent);
    if (!parentItem || row >= parentItem->children.size()) {
        return QModelIndex();
    }
    
    DotfileItem *childItem = parentItem->children.at(row);
    return createIndex(row, column, childItem);
}

QModelIndex DotfileManager::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }
    
    DotfileItem *childItem = getItem(child);
    if (!childItem) {
        return QModelIndex();
    }
    
    DotfileItem *parentItem = childItem->parent;
    if (parentItem == m_rootItem || !parentItem) {
        return QModelIndex();
    }
    
    DotfileItem *grandParent = parentItem->parent;
    if (!grandParent) {
        return QModelIndex();
    }
    
    int row = grandParent->children.indexOf(parentItem);
    return createIndex(row, 0, parentItem);
}

int DotfileManager::rowCount(const QModelIndex &parent) const
{
    DotfileItem *parentItem = getItem(parent);
    return parentItem ? parentItem->children.size() : 0;
}

int DotfileManager::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 3; // Name, Status, Type
}

QVariant DotfileManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    
    DotfileItem *item = getItem(index);
    if (!item) {
        return QVariant();
    }
    
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0: return item->name;
        case 1: return item->status;
        case 2: return item->isTemplate ? QStringLiteral("Template") : 
                       item->isDirectory ? QStringLiteral("Directory") : QStringLiteral("File");
        }
        break;
        
    case Qt::DecorationRole:
        if (index.column() == 0) {
            if (item->isDirectory) {
                return QIcon::fromTheme(QStringLiteral("folder"));
            } else if (item->isTemplate) {
                return QIcon::fromTheme(QStringLiteral("text-x-generic-template"));
            } else {
                return QIcon::fromTheme(QStringLiteral("text-x-generic"));
            }
        }
        break;
        
    case Qt::ToolTipRole:
        return item->fullPath;
    }
    
    return QVariant();
}

QVariant DotfileManager::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }
    
    switch (section) {
    case 0: return QStringLiteral("Name");
    case 1: return QStringLiteral("Status");
    case 2: return QStringLiteral("Type");
    default: return QVariant();
    }
}

QString DotfileManager::getFilePath(const QModelIndex &index) const
{
    DotfileItem *item = getItem(index);
    return item ? item->fullPath : QString();
}

bool DotfileManager::isTemplate(const QModelIndex &index) const
{
    DotfileItem *item = getItem(index);
    return item ? item->isTemplate : false;
}

DotfileManager::DotfileItem *DotfileManager::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        auto *item = static_cast<DotfileItem*>(index.internalPointer());
        if (item) {
            return item;
        }
    }
    return m_rootItem;
}

void DotfileManager::onFileChanged(const QString &path)
{
    Q_EMIT fileModified(path);
}

void DotfileManager::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)
    // Refresh the entire tree when the source directory changes
    refreshFiles();
}
