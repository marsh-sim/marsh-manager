#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "applicationdata.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    ApplicationData appData;

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
