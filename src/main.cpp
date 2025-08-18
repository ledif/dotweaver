#include <QApplication>
#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QDebug>

#include "mainwindow.h"
#include "kchezmoi_version.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    KLocalizedString::setApplicationDomain("kchezmoi");
    
    KAboutData aboutData(
        QStringLiteral("kchezmoi"),
        i18n("KChezmoi"),
        QStringLiteral(KCHEZMOI_VERSION_STRING),
        i18n("A KDE application for managing dotfiles with chezmoi"),
        KAboutLicense::GPL_V3,
        i18n("Copyright 2025, KChezmoi developers")
    );
    
    aboutData.addAuthor(
        i18n("KChezmoi Team"),
        i18n("Developer"),
        QStringLiteral("team@kchezmoi.org")
    );
    
    aboutData.setHomepage(QStringLiteral("https://github.com/kde/kchezmoi"));
    aboutData.setBugAddress(QStringLiteral("https://github.com/kde/kchezmoi/issues"));
    aboutData.setDesktopFileName(QStringLiteral("org.kde.kchezmoi"));
    
    KAboutData::setApplicationData(aboutData);
    
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
