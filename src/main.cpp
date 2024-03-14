#include <QApplication>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStyleFactory>
#include "appearanceutils.h"
#include "applicationdata.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationVersion(APP_VERSION);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.process(app);

    ApplicationData appData;

#ifdef Q_OS_WIN
    AppearanceUtils::fixWindowsPalette(&app);
#endif

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
