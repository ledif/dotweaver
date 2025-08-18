#include "dotfilemanager.h"
#include "chezmoiservice.h"
#include "logger.h"

#include <memory>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QDebug>
#include <QStringList>
#include <QColor>
#include <QMimeDatabase>
#include <QMimeType>

using namespace Qt::Literals::StringLiterals;

DotfileManager::DotfileManager(QObject *parent)
    : QAbstractItemModel(parent)
    , m_chezmoiService(nullptr)
    , m_rootItem(std::make_unique<DotfileItem>())
{
}

DotfileManager::~DotfileManager() = default;

void DotfileManager::setChezmoiService(ChezmoiService *service)
{
    m_chezmoiService = service;
    LOG_INFO(QStringLiteral("DotfileManager: ChezmoiService set to %1").arg(service ? "valid pointer"_L1 : "nullptr"_L1));
    refreshFiles();
}

void DotfileManager::refreshFiles()
{
    LOG_INFO("DotfileManager: Refreshing files..."_L1);
    
    if (!m_chezmoiService) {
        LOG_WARNING("DotfileManager: No ChezmoiService available"_L1);
        return;
    }
    
    beginResetModel();
    
    // Clear existing data
    m_rootItem = std::make_unique<DotfileItem>();
    
    buildFileTree();
    
    endResetModel();
    Q_EMIT filesRefreshed();
}

void DotfileManager::buildFileTree()
{
    if (!m_chezmoiService) {
        LOG_WARNING("DotfileManager: Cannot build file tree - no ChezmoiService"_L1);
        return;
    }
    
    LOG_INFO("DotfileManager: Building file tree..."_L1);
    auto files = m_chezmoiService->getManagedFiles();
    LOG_INFO(QStringLiteral("DotfileManager: Received %1 files from ChezmoiService").arg(files.size()));
    
    for (const auto &file : files) {
        LOG_DEBUG(QStringLiteral("DotfileManager: Adding file to tree: %1").arg(file.path));
        addFileToTree(file.path, file.sourceFile.absoluteFilePath(), file.status, file.isTemplate);
    }
    
    LOG_INFO(QStringLiteral("DotfileManager: Tree building complete, root has %1 children").arg(m_rootItem->children.size()));
}

void DotfileManager::addFileToTree(const QString &relativePath, const QString &fullPath, const QString &status, bool isTemplate)
{
    LOG_DEBUG(QStringLiteral("DotfileManager: addFileToTree called with path: %1").arg(relativePath));
    
    QStringList pathParts = relativePath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (pathParts.isEmpty()) {
        LOG_WARNING(QStringLiteral("DotfileManager: Empty path parts for: %1").arg(relativePath));
        return;
    }
    
    LOG_DEBUG(QStringLiteral("DotfileManager: Path parts: %1").arg(pathParts.join(", "_L1)));
    
    DotfileItem *currentParent = m_rootItem.get();
    
    // Navigate/create the directory structure
    for (int i = 0; i < pathParts.size() - 1; ++i) {
        currentParent = findOrCreateParent(pathParts[i], currentParent);
        LOG_DEBUG(QStringLiteral("DotfileManager: Created/found parent: %1").arg(pathParts[i]));
    }
    
    auto *fileItem = new DotfileItem(pathParts.last(), currentParent);
    fileItem->fullPath = fullPath;
    fileItem->status = status;
    fileItem->isTemplate = isTemplate;
    fileItem->isDirectory = false; // chezmoi excluded directories, so this is always a file/symlink
    
    currentParent->children.append(fileItem);
    LOG_DEBUG(QStringLiteral("DotfileManager: Added file item: %1 (parent has %2 children)").arg(pathParts.last()).arg(currentParent->children.size()));
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
    if (parentItem == m_rootItem.get() || !parentItem) {
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
    int count = parentItem ? parentItem->children.size() : 0;
    
    if (!parent.isValid()) {
        // This is the root
        LOG_DEBUG(QStringLiteral("DotfileManager: rowCount for root: %1").arg(count));
    }
    
    return count;
}

int DotfileManager::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1; // Just the name/file tree
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
            return getFileIcon(item->fullPath, item->isDirectory, item->isTemplate);
        }
        break;
        
    case Qt::ForegroundRole:
        if (index.column() == 0) {
            QColor itemColor = getItemColor(item);
            // Only return a color if it's not the default (for special status files)
            if (itemColor.isValid()) {
                return itemColor;
            }
            // For normal files, don't return anything so Qt uses theme default
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
    
    if (section == 0) {
        return QStringLiteral("Files");
    }
    
    return QVariant();
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
    return m_rootItem.get();
}

QColor DotfileManager::getItemColor(DotfileItem *item) const
{
    if (!item) {
        return QColor(); // Invalid color - use theme default
    }
    
    // Colors similar to VS Code - only return valid colors for special status
    if (item->status == u"modified"_s) {
        return QColor(255, 193, 7); // Dark gold/amber for modified files
    } else if (item->status == u"added"_s) {
        return QColor(108, 218, 118); // Green for added files
    } else if (item->status == u"deleted"_s) {
        return QColor(248, 81, 73); // Red for deleted files
    } else if (item->isDirectory && hasModifiedChildren(item)) {
        return QColor(255, 193, 7); // Dark gold for directories with modified children
    }
    
    // For normal/managed files, return invalid color so Qt uses theme default
    return QColor();
}

bool DotfileManager::hasModifiedChildren(DotfileItem *item) const
{
    if (!item) {
        return false;
    }
    
    // Check if any direct children are modified
    for (DotfileItem *child : item->children) {
        if (child->status == u"modified"_s || child->status == u"added"_s || child->status == u"deleted"_s) {
            return true;
        }
        
        // Recursively check subdirectories
        if (child->isDirectory && hasModifiedChildren(child)) {
            return true;
        }
    }
    
    return false;
}

QIcon DotfileManager::getFileIcon(const QString &filePath, bool isDirectory, bool isTemplate) const
{
    // Handle directories first
    if (isDirectory) {
        return QIcon::fromTheme(QStringLiteral("folder"));
    }
    
    // Handle template files with a special overlay or different icon
    if (isTemplate) {
        // For templates, we can either use a special template icon or the MIME type icon with template indicator
        // Let's use the MIME type icon but could add template indication later
    }
    
    // Use MIME database to get the appropriate icon
    static QMimeDatabase mimeDb;
    
    // Get MIME type from file path
    QMimeType mimeType = mimeDb.mimeTypeForFile(filePath);
    QString iconName = mimeType.iconName();
    
    // Try to get the icon from theme
    QIcon icon = QIcon::fromTheme(iconName);
    
    // If no icon found, try generic icon name
    if (icon.isNull() && !mimeType.genericIconName().isEmpty()) {
        icon = QIcon::fromTheme(mimeType.genericIconName());
    }
    
    // If still no icon, try fallbacks based on MIME type category
    if (icon.isNull()) {
        QString mimeTypeName = mimeType.name();
        if (mimeTypeName.startsWith(QStringLiteral("text/"))) {
            icon = QIcon::fromTheme(QStringLiteral("text-x-generic"));
        } else if (mimeTypeName.startsWith(QStringLiteral("image/"))) {
            icon = QIcon::fromTheme(QStringLiteral("image-x-generic"));
        } else if (mimeTypeName.startsWith(QStringLiteral("audio/"))) {
            icon = QIcon::fromTheme(QStringLiteral("audio-x-generic"));
        } else if (mimeTypeName.startsWith(QStringLiteral("video/"))) {
            icon = QIcon::fromTheme(QStringLiteral("video-x-generic"));
        }
    }
    
    // Additional fallback to specific file type icons for common dotfiles
    if (icon.isNull()) {
        QString fileName = QFileInfo(filePath).fileName().toLower();
        
        // Common shell files
        if (fileName.contains(QStringLiteral("bash")) || fileName.contains(QStringLiteral("zsh")) || 
            fileName.contains(QStringLiteral("fish")) || fileName.endsWith(QStringLiteral(".sh"))) {
            icon = QIcon::fromTheme(QStringLiteral("application-x-shellscript"));
        }
        // Git files
        else if (fileName.contains(QStringLiteral("git"))) {
            icon = QIcon::fromTheme(QStringLiteral("git"));
        }
        // Vim files
        else if (fileName.contains(QStringLiteral("vim")) || fileName.endsWith(QStringLiteral(".vim"))) {
            icon = QIcon::fromTheme(QStringLiteral("text-x-script"));
        }
        // SSH files
        else if (fileName.contains(QStringLiteral("ssh"))) {
            icon = QIcon::fromTheme(QStringLiteral("network-server"));
        }
        // Config files
        else if (fileName.contains(QStringLiteral("config")) || fileName.contains(QStringLiteral("conf"))) {
            icon = QIcon::fromTheme(QStringLiteral("preferences-other"));
        }
        // Environment files
        else if (fileName.contains(QStringLiteral("env")) || fileName.contains(QStringLiteral("profile"))) {
            icon = QIcon::fromTheme(QStringLiteral("preferences-desktop-environment"));
        }
    }
    
    // Final fallback - always return a text file icon if nothing else worked
    if (icon.isNull()) {
        icon = QIcon::fromTheme(QStringLiteral("text-x-generic"));
        // If even that fails (shouldn't happen in most themes), try alternatives
        if (icon.isNull()) {
            icon = QIcon::fromTheme(QStringLiteral("text-plain"));
        }
        if (icon.isNull()) {
            icon = QIcon::fromTheme(QStringLiteral("document-new"));
        }
    }
    
    // For template files, we could add a small overlay or badge here in the future
    // For now, templates just use the same icon as the base file type
    
    return icon;
}
