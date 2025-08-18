#include "statusbar.h"
#include "chezmoiservice.h"

#include <QLabel>
#include <QStatusBar>
#include <QProcess>
#include <QTimer>
#include <KLocalizedString>

using namespace Qt::Literals::StringLiterals;

StatusBar::StatusBar(QStatusBar *statusBar, ChezmoiService *chezmoiService, QWidget *parent)
    : QWidget(parent)
    , m_statusBar(statusBar)
    , m_chezmoiService(chezmoiService)
    , m_statusLeft(nullptr)
    , m_statusCenter(nullptr)
    , m_statusRight(nullptr)
    , m_gitUpdateTimer(std::make_unique<QTimer>(this))
{
    setupStatusWidgets();
    
    // Setup timer for periodic git status updates
    connect(m_gitUpdateTimer.get(), &QTimer::timeout, this, &StatusBar::onGitStatusTimer);
    m_gitUpdateTimer->start(300000); // Update every 300 seconds
    
    // Initial update
    updateGitStatus();
}

StatusBar::~StatusBar() = default;

void StatusBar::setupStatusWidgets()
{
    // Create status bar labels
    m_statusLeft = new QLabel(this);
    m_statusCenter = new QLabel(this);
    m_statusRight = new QLabel(this);
    
    // Set initial text - swapped left and right
    m_statusLeft->setText(i18n("Git: Loading..."));
    m_statusCenter->setText(""_L1);
    m_statusRight->setText(i18n("Ready"));
    
    // Add to status bar
    m_statusBar->addWidget(m_statusLeft, 1);
    m_statusBar->addPermanentWidget(m_statusCenter, 1);
    m_statusBar->addPermanentWidget(m_statusRight, 0);
}

void StatusBar::setLeftText(const QString &text)
{
    if (m_statusLeft) {
        m_statusLeft->setText(text);
    }
}

void StatusBar::setCenterText(const QString &text)
{
    if (m_statusCenter) {
        m_statusCenter->setText(text);
    }
}

void StatusBar::updateGitStatus()
{
    if (!m_chezmoiService || !m_statusLeft) {
        return;
    }
    
    QString gitInfo = getGitInfo();
    m_statusLeft->setText(gitInfo);
}

void StatusBar::onGitStatusTimer()
{
    updateGitStatus();
}

QString StatusBar::getGitInfo()
{
    if (!m_chezmoiService) {
        return i18n("Git: Service unavailable");
    }
    
    QString chezmoiDir = m_chezmoiService->getChezmoiDirectory();
    
    // Get the last commit hash and time
    QProcess gitProcess;
    gitProcess.setWorkingDirectory(chezmoiDir);
    
    // Get short commit hash and relative time
    gitProcess.start("git"_L1, {
        "log"_L1, "-1"_L1, "--format=%h|%ar"_L1
    });
    
    if (gitProcess.waitForFinished(3000) && gitProcess.exitCode() == 0) {
        QString output = QString::fromUtf8(gitProcess.readAllStandardOutput()).trimmed();
        QStringList parts = output.split('|'_L1);
        
        if (parts.size() == 2) {
            QString hash = parts[0];
            QString timeAgo = parts[1];
            return QStringLiteral("%1 • %2").arg(hash, timeAgo);
        } else {
            return i18n("Git: No commits");
        }
    } else {
        return i18n("Git: Not available");
    }
}
