#include "networkdisplay.h"
#include <QTimer>

NetworkDisplay::NetworkDisplay(QObject *parent)
    : QObject{parent}
{
    _model = new QStandardItemModel(this);

    auto parentItem = _model->invisibleRootItem();
    for (int i = 0; i < 4; ++i) {
        auto item = new QStandardItem(QString("item %0").arg(i));
        parentItem->appendRow(item);
        auto second = new QStandardItem(QString("second %0").arg(i));
        parentItem->appendColumn({second});

        parentItem = item;
    }
    auto added = new QStandardItem(QString("added"));
    auto addedSecond = new QStandardItem(QString("added second"));
    parentItem->appendRow({added, addedSecond});
    _model->setHorizontalHeaderLabels({"Name", "Details"});
}

void NetworkDisplay::itemClicked(const QModelIndex &index)
{
    auto item = _model->itemFromIndex(index);
    auto deb = qDebug().nospace();
    deb << "Clicked item { ";
    for (const auto role : _model->roleNames().keys()) {
        deb << _model->roleNames()[role] << ": " << item->data(role) << ", ";
    }
    deb << "}";
}
