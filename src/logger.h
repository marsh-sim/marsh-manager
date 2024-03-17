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
    Q_PROPERTY(
        QString fileComment READ fileComment WRITE setFileComment NOTIFY fileCommentChanged FINAL)
    Q_PROPERTY(double bytesWritten READ bytesWritten NOTIFY bytesWrittenChanged FINAL)

    Q_INVOKABLE QString getDirectoryWithDialog();

    bool savingNow() const;
    QString outputDir() const;
    QString outputPath() const;
    QString fileComment() const;
    double bytesWritten() const; ///< property is double to pass NaN to QML when no file was opened

    void setSavingNow(bool saving);
    void setOutputDir(QString dir);
    void setFileComment(const QString &newFileComment);

signals:
    void savingNowChanged(bool saving);
    void outputDirChanged(QString dir);
    void outputPathChanged(QString path);
    void fileCommentChanged(QString fileComment);
    void bytesWrittenChanged(double written);

private slots:
    void receiveMessage(Message message);

private:
    QString formatFilename(std::optional<QDateTime> datetime = std::nullopt) const;

    ApplicationData *appData;
    QDir _outputDir;
    QFile *outputFile = nullptr;
    QString _fileComment;
    std::optional<qint64> _bytesWritten;
};

#endif // LOGGER_H
