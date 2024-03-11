#ifndef APPLICATIONDATA_H
#define APPLICATIONDATA_H

#include <QObject>
#include "networkdisplay.h"
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
    Q_PROPERTY(QString licenseText READ licenseText CONSTANT)

    Router *router() const { return _router; }
    NetworkDisplay *networkDisplay() const { return _networkDisplay; }
    QString licenseText();

signals:

private:
    Router* _router;
    NetworkDisplay *_networkDisplay;
};

#endif // APPLICATIONDATA_H
