#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "appearanceutils.h"
#include "applicationdata.h"
#include "git_version.h"
#include "message.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationVersion(QString("%1+%2.%3").arg(APP_VERSION).arg(kGitCount).arg(kGitHash));

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.process(app);

    ApplicationData appData;
    appData.dialect()->loadDefinitions(); // turned out fast enough to call it here

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

    // Start the message timer
    Message::timeBootMs();

    return app.exec();
}
