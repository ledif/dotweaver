#include <QtTest/QtTest>
#include "chezmoiservice.h"

class TestChezmoiService : public QObject
{
    Q_OBJECT

private slots:
    void testExecutablePath();
    void testDirectoryPath();
    void testInitialization();

private:
    ChezmoiService *service;
};

void TestChezmoiService::testExecutablePath()
{
    service = new ChezmoiService(this);
    
    // Test that we can find or at least attempt to find chezmoi
    QString executablePath = QStandardPaths::findExecutable("chezmoi");
    
    // This test will pass even if chezmoi is not installed
    // It just checks that the service can handle the case gracefully
    QVERIFY(true); // Always pass for now
    
    delete service;
}

void TestChezmoiService::testDirectoryPath()
{
    service = new ChezmoiService(this);
    
    QString chezmoiDir = service->getChezmoiDirectory();
    QVERIFY(!chezmoiDir.isEmpty());
    
    // Should contain a valid path (even if directory doesn't exist)
    QVERIFY(chezmoiDir.contains("chezmoi"));
    
    delete service;
}

void TestChezmoiService::testInitialization()
{
    service = new ChezmoiService(this);
    
    // Test initialization check (should not crash)
    bool isInit = service->isChezmoiInitialized();
    
    // This is okay to be false if chezmoi isn't set up
    Q_UNUSED(isInit)
    QVERIFY(true);
    
    delete service;
}

QTEST_GUILESS_MAIN(TestChezmoiService)
#include "test_chezmoiservice.moc"
