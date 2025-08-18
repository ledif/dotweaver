#include <QtTest/QtTest>
#include <QApplication>
#include "dotfilemanager.h"
#include "chezmoiservice.h"

class TestDotfileManager : public QObject
{
    Q_OBJECT

private slots:
    void testModelStructure();
    void testFileHandling();

private:
    DotfileManager *manager;
    ChezmoiService *service;
};

void TestDotfileManager::testModelStructure()
{
    // Create a QApplication instance for GUI tests
    int argc = 0;
    char *argv[] = {nullptr};
    QApplication app(argc, argv);
    
    service = new ChezmoiService(this);
    manager = new DotfileManager(this);
    
    // Test basic model structure
    QVERIFY(manager->rowCount() >= 0);
    QCOMPARE(manager->columnCount(), 3);
    
    // Test header data
    QCOMPARE(manager->headerData(0, Qt::Horizontal).toString(), QString("Name"));
    QCOMPARE(manager->headerData(1, Qt::Horizontal).toString(), QString("Status"));
    QCOMPARE(manager->headerData(2, Qt::Horizontal).toString(), QString("Type"));
    
    delete manager;
    delete service;
}

void TestDotfileManager::testFileHandling()
{
    int argc = 0;
    char *argv[] = {nullptr};
    QApplication app(argc, argv);
    
    service = new ChezmoiService(this);
    manager = new DotfileManager(this);
    
    manager->setChezmoiService(service);
    
    // Test that setting service doesn't crash
    QVERIFY(true);
    
    // Test path retrieval with invalid index
    QString path = manager->getFilePath(QModelIndex());
    QVERIFY(path.isEmpty());
    
    delete manager;
    delete service;
}

QTEST_GUILESS_MAIN(TestDotfileManager)
#include "test_dotfilemanager.moc"
