#ifndef APPLICATIONDATA_H
#define APPLICATIONDATA_H

#include <QObject>
#include "dialectinfo.h"
#include "heartbeatservice.h"
#include "logger.h"
#include "networkdisplay.h"
#include "networkdisplayproxy.h"
#include "parameterservice.h"
#include "router.h"

/// Central object available in QML through `appData` property
/// Based on https://doc.qt.io/qt-6.5/qtqml-cppintegration-contextproperties.html#setting-an-object-as-a-context-property
class ApplicationData : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationData(QObject *parent = nullptr);

    Q_PROPERTY(Router* router READ router CONSTANT)
    Q_PROPERTY(NetworkDisplay *networkDisplay READ networkDisplay CONSTANT)
    Q_PROPERTY(NetworkDisplayProxy *displayModel READ displayModel CONSTANT)
    Q_PROPERTY(Logger *logger READ logger CONSTANT)
    Q_PROPERTY(DialectInfo *dialect READ dialect CONSTANT)
    Q_PROPERTY(ParameterService *parameterService READ parameterService CONSTANT)

    Q_PROPERTY(quint8 localSystemId READ localSystemId CONSTANT)
    Q_PROPERTY(quint8 localComponentId READ localComponentId CONSTANT)

    Q_PROPERTY(QString buildType READ buildType CONSTANT)
    Q_PROPERTY(QString buildGitCommitCount READ buildGitCommitCount CONSTANT)
    Q_PROPERTY(QString buildGitHash READ buildGitHash CONSTANT)
    Q_PROPERTY(QString licenseText READ licenseText CONSTANT)

    Router *router() const { return _router; }
    NetworkDisplay *networkDisplay() const { return _networkDisplay; }
    NetworkDisplayProxy *displayModel() const { return _displayModel; }
    Logger *logger() const { return _logger; }
    DialectInfo *dialect() const { return _dialect; }
    ParameterService *parameterService() const { return _parameterService; };

    quint8 localSystemId() const;
    quint8 localComponentId() const;

    QString buildType() const { return APP_BUILD_TYPE; }
    QString buildGitCommitCount() const { return APP_GIT_COMMIT_COUNT; }
    QString buildGitHash() const { return APP_GIT_HASH; }
    QString licenseText();

signals:

private:
    Router* _router;
    Logger *_logger;
    NetworkDisplay *_networkDisplay;
    NetworkDisplayProxy *_displayModel;
    DialectInfo *_dialect;

    // microservice providers
    HeartbeatService *_heartbeatService;
    ParameterService *_parameterService;
};

#endif // APPLICATIONDATA_H
