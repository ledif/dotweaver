#ifndef LOGVIEWER_H
#define LOGVIEWER_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
QT_END_NAMESPACE

class LogViewer : public QDialog
{
    Q_OBJECT

public:
    explicit LogViewer(QWidget *parent = nullptr);

private Q_SLOTS:
    void refreshLog();
    void clearLog();
    void saveLog();

private:
    void setupUI();
    
    QTextEdit *m_logTextEdit;
    QPushButton *m_refreshButton;
    QPushButton *m_clearButton;
    QPushButton *m_saveButton;
    QPushButton *m_closeButton;
};

#endif // LOGVIEWER_H
