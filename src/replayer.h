#ifndef REPLAYER_H
#define REPLAYER_H

#include <QDir>
#include <QFile>
#include <QObject>
#include <QTimer>
#include "message.h"

class ApplicationData;

class Replayer : public QObject
{
    Q_OBJECT
public:
    explicit Replayer(QObject *parent = nullptr);
    void setAppData(ApplicationData *appData);

    Q_PROPERTY(bool isReplaying READ isReplaying NOTIFY isReplayingChanged FINAL)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged FINAL)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged FINAL)
    Q_PROPERTY(double playbackSpeed READ playbackSpeed WRITE setPlaybackSpeed NOTIFY playbackSpeedChanged FINAL)
    Q_PROPERTY(bool isPaused READ isPaused NOTIFY isPausedChanged FINAL)
    Q_PROPERTY(double totalDuration READ totalDuration NOTIFY totalDurationChanged FINAL)
    Q_PROPERTY(bool canStartReplay READ canStartReplay NOTIFY canStartReplayChanged FINAL)

    Q_INVOKABLE QString selectFileWithDialog();
    Q_INVOKABLE bool startReplay(const QString &filePath);
    Q_INVOKABLE void pauseReplay();
    Q_INVOKABLE void resumeReplay();
    Q_INVOKABLE void stopReplay();
    Q_INVOKABLE bool validateConnectedNodes();
    Q_INVOKABLE void checkCanStartReplay();
    Q_INVOKABLE void seekToProgress(double progress);

    bool isReplaying() const { return _isReplaying; }
    QString currentFile() const { return _currentFile; }
    double progress() const { return _progress; }
    double playbackSpeed() const { return _playbackSpeed; }
    bool isPaused() const { return _isPaused; }
    double totalDuration() const { return _totalDuration; }
    bool canStartReplay() const { return _canStartReplay; }

    void setPlaybackSpeed(double speed);

signals:
    void isReplayingChanged(bool isReplaying);
    void currentFileChanged(QString filePath);
    void progressChanged(double progress);
    void playbackSpeedChanged(double speed);
    void isPausedChanged(bool isPaused);
    void totalDurationChanged(double duration);
    void errorOccurred(QString error);
    void replayFinished();
    void canStartReplayChanged(bool canStart);

private slots:
    void sendNextMessage();

private:
    struct MessageEntry {
        qint64 timestamp;
        mavlink_message_t message;
    };

    bool loadFile(const QString &filePath);
    void cleanup();

    ApplicationData *appData = nullptr;
    QFile *replayFile = nullptr;
    QString _currentFile;
    bool _isReplaying = false;
    bool _isPaused = false;
    bool _canStartReplay = false;
    double _playbackSpeed = 1.0;
    double _progress = 0.0;
    double _totalDuration = 0.0;

    QList<MessageEntry> messages;
    qsizetype currentMessageIndex = 0;
    qint64 replayStartTime = 0;
    qint64 firstMessageTimestamp = 0;
    QTimer *playbackTimer = nullptr;
};

#endif // REPLAYER_H
