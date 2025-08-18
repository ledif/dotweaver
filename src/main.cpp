#include <QApplication>
#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QDebug>

#include "mainwindow.h"
#include "dotweaver_version.h"

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
        KAboutLicense::GPL_V3,
        i18n("Copyright 2025, DotWeaver contributors")
    );
    
    aboutData.addAuthor(
        i18n("DotWeaver Team"),
        i18n("Developer"),
        QStringLiteral("team@dotweaver.app")
    );
    
    aboutData.setHomepage(QStringLiteral("https://github.com/dotweaver/dotweaver"));
    aboutData.setBugAddress("https://github.com/dotweaver/dotweaver/issues");
    aboutData.setDesktopFileName(QStringLiteral("org.dotweaver"));
    
    KAboutData::setApplicationData(aboutData);
    
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
