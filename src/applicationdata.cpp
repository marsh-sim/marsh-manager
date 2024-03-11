#include "applicationdata.h"

#include <QFile>

ApplicationData::ApplicationData(QObject *parent)
    : QObject{parent}
{
    _router = new Router(this);
    _networkDisplay = new NetworkDisplay(this);
}

QString ApplicationData::licenseText()
{
    QString filename(":/LICENSE.txt");

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open license file";
        return "";
    }

    return file.readAll();
}
