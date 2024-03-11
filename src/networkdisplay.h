#ifndef NETWORKMODEL_H
#define NETWORKMODEL_H

#include <QObject>
#include <QStandardItemModel>

class NetworkDisplay : public QObject
{
    Q_OBJECT
public:
    explicit NetworkDisplay(QObject *parent = nullptr);

    Q_PROPERTY(QStandardItemModel *model READ model CONSTANT)

    QStandardItemModel *model() const { return _model; }

signals:

public slots:
    void itemClicked(const QModelIndex &index);

private:
    QStandardItemModel *_model;
};

#endif // NETWORKMODEL_H
