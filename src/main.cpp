#include <QApplication>
#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QDebug>

#include "mainwindow.h"
#include "logger.h"
#include "dotweaver_version.h"

using namespace Qt::Literals::StringLiterals;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application icon
    QIcon appIcon = QIcon::fromTheme(QStringLiteral("dotweaver"), QIcon(QStringLiteral(":/icons/dotweaver.png")));
    app.setWindowIcon(appIcon);
    
    KLocalizedString::setApplicationDomain("dotweaver");
    
    KAboutData aboutData(
        QStringLiteral("dotweaver"),
        i18n("DotWeaver"),
        QStringLiteral(DOTWEAVER_VERSION_STRING),
        i18n("A modern dotfile management application powered by chezmoi"),
        KAboutLicense::MIT,
        i18n("(c) 2025 Adam Fidel")
    );
    
    aboutData.addAuthor(
        i18n("DotWeaver Team"),
        i18n("Adam Fidel"),
        QStringLiteral("adam@fidel.cloud")
    );
    
    aboutData.setHomepage(QStringLiteral("https://github.com/ledif/dotweaver"));
    aboutData.setBugAddress("https://github.com/ledif/dotweaver/issues");
    aboutData.setDesktopFileName(QStringLiteral("io.github.ledif.dotweaver"));
    
    KAboutData::setApplicationData(aboutData);
    
    // Initialize logging
    Logger::instance()->setupLogging();
    //LOG_INFO("DotWeaver application starting"_L1);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
