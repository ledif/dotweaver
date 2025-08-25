#ifndef DATAVIEWER_H
#define DATAVIEWER_H

#include <QDialog>
#include <QJsonObject>
#include <QJsonValue>
#include <memory>

class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QSplitter;
class QTextEdit;
class ChezmoiService;

/**
 * @brief Dialog window for displaying chezmoi template data in a structured format
 */
class DataViewer : public QDialog
{
    Q_OBJECT

public:
    explicit DataViewer(const QString &jsonData, QWidget *parent = nullptr);
    ~DataViewer() override = default;

public Q_SLOTS:
    void refreshData();

private Q_SLOTS:
    void onItemSelectionChanged();
    void copySelectedValue();
    void copySelectedPath();

private:
    void setupUI();
    void loadJsonData(const QString &jsonData);
    void loadChezmoiData();
    void populateTreeFromJson(const QJsonObject &json, QTreeWidgetItem *parentItem = nullptr);
    void addJsonValueToTree(const QString &key, const QJsonValue &value, QTreeWidgetItem *parentItem);
    QString getJsonPath(QTreeWidgetItem *item) const;
    QString formatJsonValue(const QJsonValue &value) const;

    ChezmoiService *m_chezmoiService;
    QTreeWidget *m_treeWidget;
    QTextEdit *m_detailsEdit;
    QSplitter *m_splitter;
    QPushButton *m_refreshButton;
    QPushButton *m_copyValueButton;
    QPushButton *m_copyPathButton;
    QPushButton *m_closeButton;
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_buttonLayout;
    
    QJsonObject m_data;
};

#endif // DATAVIEWER_H
