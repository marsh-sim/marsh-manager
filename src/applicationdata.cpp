#include "applicationdata.h"

ApplicationData::ApplicationData(QObject *parent)
    : QObject{parent}
{
    _router = new Router(this);
}
