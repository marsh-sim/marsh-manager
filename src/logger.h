#ifndef LOGGER_H
#define LOGGER_H

#include <QDir>
#include <QFile>
#include <QObject>
#include "message.h"

class ApplicationData;

class Logger : public QObject
{
    Q_OBJECT
public:
    explicit Logger(QObject *parent = nullptr);
    void setAppData(ApplicationData *appData);

    Q_PROPERTY(bool savingNow READ savingNow WRITE setSavingNow NOTIFY savingNowChanged FINAL)
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged FINAL)
    Q_PROPERTY(QString outputPath READ outputPath NOTIFY outputPathChanged FINAL)

    Q_INVOKABLE QString getDirectoryWithDialog();

    bool savingNow() const;
    QString outputDir() const;
    QString outputPath() const;

    void setSavingNow(bool saving);
    void setOutputDir(QString dir);
signals:
    void savingNowChanged(bool saving);
    void outputDirChanged(QString dir);
    void outputPathChanged(QString path);

private slots:
    void receiveMessage(Message message);

private:
    QDir _outputDir;
    QFile *outputFile = nullptr;
};

#endif // LOGGER_H
