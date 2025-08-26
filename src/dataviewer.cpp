#include "dataviewer.h"
#include "chezmoiservice.h"
#include "logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTextEdit>
#include <QPushButton>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QFont>
#include <QFontDatabase>
#include <KLocalizedString>
#include <QIcon>

using namespace Qt::Literals::StringLiterals;

DataViewer::DataViewer(const QString &jsonData, QWidget *parent)
    : QDialog(parent)
    , m_chezmoiService(nullptr)
    , m_treeWidget(nullptr)
    , m_detailsEdit(nullptr)
    , m_splitter(nullptr)
    , m_expandButton(nullptr)
    , m_copyValueButton(nullptr)
    , m_copyPathButton(nullptr)
    , m_mainLayout(nullptr)
    , m_buttonLayout(nullptr)
{
    setWindowTitle(i18n("Chezmoi Template Data"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("code-context")));
    resize(800, 600);
    
    setupUI();
    loadJsonData(jsonData);
}

void DataViewer::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    // Create splitter for tree and details
    m_splitter = new QSplitter(Qt::Horizontal, this);
    
    // Create tree widget
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels({i18n("Key"), i18n("Type"), i18n("Value")});
    m_treeWidget->header()->setStretchLastSection(false);
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setSortingEnabled(false);
    
    connect(m_treeWidget, &QTreeWidget::itemSelectionChanged,
            this, &DataViewer::onItemSelectionChanged);
    
    // Create details text edit
    m_detailsEdit = new QTextEdit(this);
    m_detailsEdit->setReadOnly(true);
    m_detailsEdit->setPlainText(i18n("Select an item to view details"));
    
    // Use monospace font for JSON display
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_detailsEdit->setFont(monoFont);
    
    // Add widgets to splitter
    m_splitter->addWidget(m_treeWidget);
    m_splitter->addWidget(m_detailsEdit);
    m_splitter->setSizes({640, 160}); // 80% tree view, 20% details view
    
    m_mainLayout->addWidget(m_splitter);
    
    // Create button layout
    m_buttonLayout = new QHBoxLayout();
    
    m_expandButton = new QPushButton(i18n("&Expand All"), this);
    m_expandButton->setIcon(QIcon::fromTheme(QStringLiteral("view-list-tree")));
    m_expandButton->setToolTip(i18n("Expand all items in the tree view"));
    connect(m_expandButton, &QPushButton::clicked, this, &DataViewer::expandAllItems);
    
    m_copyValueButton = new QPushButton(i18n("Copy &Value"), this);
    m_copyValueButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    m_copyValueButton->setToolTip(i18n("Copy the selected value to clipboard"));
    m_copyValueButton->setEnabled(false);
    connect(m_copyValueButton, &QPushButton::clicked, this, &DataViewer::copySelectedValue);
    
    m_copyPathButton = new QPushButton(i18n("Copy &Path"), this);
    m_copyPathButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    m_copyPathButton->setToolTip(i18n("Copy the template path (e.g., .chezmoi.hostname) to clipboard"));
    m_copyPathButton->setEnabled(false);
    connect(m_copyPathButton, &QPushButton::clicked, this, &DataViewer::copySelectedPath);
    
    // Add stretch to push buttons to the left
    m_buttonLayout->addWidget(m_expandButton);
    m_buttonLayout->addWidget(m_copyValueButton);
    m_buttonLayout->addWidget(m_copyPathButton);
    m_buttonLayout->addStretch();
    
    m_mainLayout->addLayout(m_buttonLayout);
}

void DataViewer::loadJsonData(const QString &jsonData)
{
    if (jsonData.isEmpty()) {
        LOG_ERROR("No JSON data provided"_L1);
        QMessageBox::warning(this, i18n("Error"), i18n("No template data provided"));
        return;
    }
    
    LOG_INFO("Loading chezmoi template data"_L1);
    
    // Parse JSON
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData.toUtf8(), &error);
    
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QStringLiteral("JSON parse error: %1").arg(error.errorString()));
        QMessageBox::warning(this, i18n("Error"), 
                           i18n("Failed to parse template data: %1").arg(error.errorString()));
        return;
    }
    
    if (!doc.isObject()) {
        LOG_ERROR("Template data is not a JSON object"_L1);
        QMessageBox::warning(this, i18n("Error"), i18n("Template data is not in expected format"));
        return;
    }
    
    m_data = doc.object();
    
    // Clear existing tree
    m_treeWidget->clear();
    
    // Populate tree
    populateTreeFromJson(m_data);
    
    // Expand the first level
    m_treeWidget->expandToDepth(0);
    
    LOG_INFO("Successfully loaded template data"_L1);
}

void DataViewer::populateTreeFromJson(const QJsonObject &json, QTreeWidgetItem *parentItem)
{
    for (auto it = json.begin(); it != json.end(); ++it) {
        addJsonValueToTree(it.key(), it.value(), parentItem);
    }
}

void DataViewer::addJsonValueToTree(const QString &key, const QJsonValue &value, QTreeWidgetItem *parentItem)
{
    QTreeWidgetItem *item;
    if (parentItem) {
        item = new QTreeWidgetItem(parentItem);
    } else {
        item = new QTreeWidgetItem(m_treeWidget);
    }
    
    item->setText(0, key);
    
    QString typeStr;
    QString valueStr;
    
    switch (value.type()) {
    case QJsonValue::Null:
        typeStr = i18n("null");
        valueStr = i18n("null");
        break;
    case QJsonValue::Bool:
        typeStr = i18n("boolean");
        valueStr = value.toBool() ? i18n("true") : i18n("false");
        break;
    case QJsonValue::Double:
        typeStr = i18n("number");
        valueStr = QString::number(value.toDouble());
        break;
    case QJsonValue::String:
        typeStr = i18n("string");
        valueStr = value.toString();
        // Truncate long strings for display
        if (valueStr.length() > 100) {
            valueStr = valueStr.left(97) + QStringLiteral("...");
        }
        break;
    case QJsonValue::Array:
        typeStr = i18n("array");
        valueStr = i18n("[%1 items]").arg(value.toArray().size());
        
        // Add array items as children
        {
            QJsonArray array = value.toArray();
            for (int i = 0; i < array.size(); ++i) {
                addJsonValueToTree(QStringLiteral("[%1]").arg(i), array.at(i), item);
            }
        }
        break;
    case QJsonValue::Object:
        typeStr = i18n("object");
        valueStr = i18n("{%1 properties}").arg(value.toObject().size());
        
        // Add object properties as children
        populateTreeFromJson(value.toObject(), item);
        break;
    case QJsonValue::Undefined:
        typeStr = i18n("undefined");
        valueStr = i18n("undefined");
        break;
    }
    
    item->setText(1, typeStr);
    item->setText(2, valueStr);
    
    // Store the original JSON value in the item data
    item->setData(0, Qt::UserRole, value.toVariant());
}

void DataViewer::onItemSelectionChanged()
{
    QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
    
    if (!currentItem) {
        m_detailsEdit->setPlainText(i18n("No item selected"));
        m_copyValueButton->setEnabled(false);
        m_copyPathButton->setEnabled(false);
        return;
    }
    
    m_copyValueButton->setEnabled(true);
    m_copyPathButton->setEnabled(true);
    
    // Get the JSON path
    QString path = getJsonPath(currentItem);
    
    // Get the value
    QVariant valueVariant = currentItem->data(0, Qt::UserRole);
    QJsonValue value = QJsonValue::fromVariant(valueVariant);
    
    // Format details text
    QString details;
    details += i18n("Path: %1\n").arg(path);
    details += i18n("Type: %1\n").arg(currentItem->text(1));
    details += i18n("Value:\n%1").arg(formatJsonValue(value));
    
    m_detailsEdit->setPlainText(details);
}

QString DataViewer::getJsonPath(QTreeWidgetItem *item) const
{
    if (!item) {
        return QString();
    }
    
    QStringList pathParts;
    QTreeWidgetItem *current = item;
    
    while (current) {
        QString part = current->text(0);
        
        // Handle array indices
        if (part.startsWith(u'[') && part.endsWith(u']')) {
            pathParts.prepend(part);
        } else {
            pathParts.prepend(part);
        }
        
        current = current->parent();
    }
    
    // Join with dots, but handle array indices specially
    QString result;
    for (int i = 0; i < pathParts.size(); ++i) {
        if (i > 0 && !pathParts[i].startsWith(u'[')) {
            result += u'.';
        }
        result += pathParts[i];
    }
    
    return result;
}

QString DataViewer::formatJsonValue(const QJsonValue &value) const
{
    QJsonDocument doc;
    
    switch (value.type()) {
    case QJsonValue::Object:
        doc.setObject(value.toObject());
        break;
    case QJsonValue::Array:
        doc.setArray(value.toArray());
        break;
    default:
        // For simple values, just return the string representation
        if (value.isString()) {
            return value.toString();
        } else if (value.isBool()) {
            return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
        } else if (value.isDouble()) {
            return QString::number(value.toDouble());
        } else if (value.isNull()) {
            return QStringLiteral("null");
        } else {
            return QStringLiteral("undefined");
        }
    }
    
    return QString::fromUtf8(doc.toJson());
}

void DataViewer::copySelectedValue()
{
    QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
    if (!currentItem) {
        return;
    }
    
    QVariant valueVariant = currentItem->data(0, Qt::UserRole);
    QJsonValue value = QJsonValue::fromVariant(valueVariant);
    QString valueText = formatJsonValue(value);
    
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(valueText);
    
    LOG_INFO(QStringLiteral("Copied value to clipboard: %1").arg(valueText.left(50)));
}

void DataViewer::copySelectedPath()
{
    QTreeWidgetItem *currentItem = m_treeWidget->currentItem();
    if (!currentItem) {
        return;
    }
    
    QString path = getJsonPath(currentItem);
    
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(path);
    
    LOG_INFO(QStringLiteral("Copied path to clipboard: %1").arg(path));
}

void DataViewer::expandAllItems()
{
    if (m_treeWidget) {
        m_treeWidget->expandAll();
        LOG_INFO("Expanded all items in the tree view"_L1);
    }
}
