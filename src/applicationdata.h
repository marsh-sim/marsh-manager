#ifndef APPLICATIONDATA_H
#define APPLICATIONDATA_H

#include "src/router.h"
#include <QObject>

/// Central object available in QML through `appData` property
/// Based on https://doc.qt.io/qt-6.5/qtqml-cppintegration-contextproperties.html#setting-an-object-as-a-context-property
class ApplicationData : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationData(QObject *parent = nullptr);

    Q_PROPERTY(Router* router READ router CONSTANT)
    Router* router() const { return _router; };
signals:

private:
    Router* _router;
};

#endif // APPLICATIONDATA_H
