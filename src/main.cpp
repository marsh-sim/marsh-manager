#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "applicationdata.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ApplicationData appData;

    // FIXME: This causes light palette on Windows (but dark palette is more broken)
    QQuickStyle::setStyle("fusion");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.rootContext()->setContextProperty("appData", &appData);
    engine.loadFromModule("marsh-manager", "Main");

    return app.exec();
}
