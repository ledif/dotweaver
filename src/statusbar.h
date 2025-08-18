#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QWidget>
#include <QTimer>
#include <memory>

class QLabel;
class QStatusBar;
class ChezmoiService;

class StatusBar : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBar(QStatusBar *statusBar, ChezmoiService *chezmoiService, QWidget *parent = nullptr);
    ~StatusBar() override;

    void setLeftText(const QString &text);
    void setCenterText(const QString &text);
    void updateGitStatus();

private Q_SLOTS:
    void onGitStatusTimer();

private:
    void setupStatusWidgets();
    QString getGitInfo();

    QStatusBar *m_statusBar;
    ChezmoiService *m_chezmoiService;
    
    QLabel *m_statusLeft;
    QLabel *m_statusCenter;
    QLabel *m_statusRight;
    
    std::unique_ptr<QTimer> m_gitUpdateTimer;
};

#endif // STATUSBAR_H
