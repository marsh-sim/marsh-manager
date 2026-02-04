#include "replayer.h"
#include "applicationdata.h"
#include "mavlink/all/mavlink.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QtEndian>

Replayer::Replayer(QObject *parent)
    : QObject{parent}
{
    playbackTimer = new QTimer(this);
    playbackTimer->setSingleShot(false);
    playbackTimer->setInterval(10); // Check every 10ms for messages to send
    connect(playbackTimer, &QTimer::timeout, this, &Replayer::sendNextMessage);
}

void Replayer::setAppData(ApplicationData *appData)
{
    this->appData = appData;
}

QString Replayer::selectFileWithDialog()
{
    QDir defaultDir = QDir::home();
    defaultDir.cd("marsh-logs");

    QString filePath = QFileDialog::getOpenFileName(nullptr,
                                        "Select replay file",
                                        defaultDir.absolutePath(),
                                        "TLog files (*.tlog);;All files (*)");
    
    if (!filePath.isEmpty()) {
        _currentFile = filePath;
        emit currentFileChanged(_currentFile);
        checkCanStartReplay();
    }
    
    return filePath;
}

bool Replayer::validateConnectedNodes()
{
    if (!appData || !appData->router())
        return false;

    const auto &clients = appData->router()->getClients();
    bool hasVisualization = false;
    
    // Check all connected clients
    for (const auto *client : clients) {
        if (client->state() == ClientNode::State::Connected) {
            if (client->type == ComponentType(MARSH_TYPE_VISUALISATION)) {
                hasVisualization = true;
            } else {
                auto typeName = appData->dialect()->componentName(client->type);
                QString typeStr = typeName ? *typeName : QString("Type %1").arg(client->type.value());
                emit errorOccurred(QString("Only VISUALIZATION nodes are allowed during replay. Found: %1").arg(typeStr));
                return false;
            }
        }
    }
    
    if (!hasVisualization) {
        emit errorOccurred("No VISUALIZATION nodes connected. At least one is required for replay.");
        return false;
    }

    return true;
}

void Replayer::checkCanStartReplay()
{
    bool canStart = !_currentFile.isEmpty() && 
                    !_isReplaying &&
                    !appData->logger()->savingNow();
    
    if (canStart) {
        // Silently check node validation without emitting errors
        if (!appData || !appData->router()) {
            canStart = false;
        } else {
            const auto &clients = appData->router()->getClients();
            bool hasVisualization = false;
            bool hasOtherNodes = false;
            
            for (const auto *client : clients) {
                if (client->state() == ClientNode::State::Connected) {
                    if (client->type == ComponentType(MARSH_TYPE_VISUALISATION)) {
                        hasVisualization = true;
                    } else {
                        hasOtherNodes = true;
                        break;
                    }
                }
            }
            
            canStart = hasVisualization && !hasOtherNodes;
        }
    }
    
    if (_canStartReplay != canStart) {
        _canStartReplay = canStart;
        emit canStartReplayChanged(_canStartReplay);
    }
}

bool Replayer::startReplay(const QString &filePath)
{
    if (_isReplaying) {
        emit errorOccurred("Already replaying. Stop current replay first.");
        return false;
    }

    if (appData->logger()->savingNow()) {
        emit errorOccurred("Cannot replay while recording. Stop recording first.");
        return false;
    }

    if (!validateConnectedNodes()) {
        return false;
    }

    if (!loadFile(filePath)) {
        return false;
    }

    _isReplaying = true;
    _isPaused = false;
    _progress = 0.0;
    currentMessageIndex = 0;
    replayStartTime = Message::currentTime();
    
    if (!messages.isEmpty()) {
        firstMessageTimestamp = messages.first().timestamp;
        qint64 lastTimestamp = messages.last().timestamp;
        _totalDuration = (lastTimestamp - firstMessageTimestamp) / 1000000.0; // Convert to seconds
        emit totalDurationChanged(_totalDuration);
    }

    playbackTimer->start();
    
    emit isReplayingChanged(_isReplaying);
    emit isPausedChanged(_isPaused);
    emit currentFileChanged(_currentFile);
    
    qInfo() << "Started replay of" << _currentFile << "with" << messages.size() << "messages";

    return true;
}

void Replayer::pauseReplay()
{
    if (!_isReplaying || _isPaused)
        return;

    _isPaused = true;
    playbackTimer->stop();
    emit isPausedChanged(_isPaused);
}

void Replayer::resumeReplay()
{
    if (!_isReplaying || !_isPaused)
        return;

    _isPaused = false;
    // Adjust replay start time to account for pause duration
    qint64 currentTime = Message::currentTime();
    qint64 elapsedReplayTime = 0;
    if (currentMessageIndex > 0) {
        elapsedReplayTime = (messages[currentMessageIndex - 1].timestamp - firstMessageTimestamp) / _playbackSpeed;
    }
    replayStartTime = currentTime - elapsedReplayTime;
    
    playbackTimer->start();
    emit isPausedChanged(_isPaused);
}

void Replayer::stopReplay()
{
    if (!_isReplaying)
        return;

    playbackTimer->stop();
    cleanup();
    
    _isReplaying = false;
    _isPaused = false;
    _progress = 0.0;
    currentMessageIndex = 0;
    
    emit isReplayingChanged(_isReplaying);
    emit isPausedChanged(_isPaused);
    emit progressChanged(_progress);
    
    qInfo() << "Stopped replay";
}

void Replayer::setPlaybackSpeed(double speed)
{
    if (speed <= 0.0 || speed > 10.0) {
        qWarning() << "Invalid playback speed:" << speed;
        return;
    }

    if (qFuzzyCompare(_playbackSpeed, speed))
        return;

    // Adjust replay start time when changing speed during playback
    if (_isReplaying && !_isPaused && currentMessageIndex > 0) {
        qint64 currentTime = Message::currentTime();
        qint64 elapsedReplayTime = (messages[currentMessageIndex - 1].timestamp - firstMessageTimestamp) / _playbackSpeed;
        replayStartTime = currentTime - (elapsedReplayTime * speed / _playbackSpeed);
    }

    _playbackSpeed = speed;
    emit playbackSpeedChanged(_playbackSpeed);
}

void Replayer::sendNextMessage()
{
    if (!_isReplaying || _isPaused || currentMessageIndex >= messages.size()) {
        if (currentMessageIndex >= messages.size()) {
            // Replay finished
            stopReplay();
            emit replayFinished();
        }
        return;
    }

    qint64 currentTime = Message::currentTime();
    qint64 elapsedRealTime = currentTime - replayStartTime;
    
    // Send all messages that should have been sent by now
    while (currentMessageIndex < messages.size()) {
        const auto &entry = messages[currentMessageIndex];
        qint64 messageDelay = (entry.timestamp - firstMessageTimestamp) / _playbackSpeed;
        
        if (messageDelay <= elapsedRealTime) {
            // Send this message
            Message msg{currentTime, entry.message};
            appData->router()->sendMessage(msg, ComponentId::Broadcast, SystemId::Broadcast);
            
            currentMessageIndex++;
            
            // Update progress
            if (messages.size() > 0) {
                _progress = static_cast<double>(currentMessageIndex) / messages.size();
                emit progressChanged(_progress);
            }
        } else {
            // This message is not ready yet
            break;
        }
    }
    
    // Check if we've finished
    if (currentMessageIndex >= messages.size()) {
        stopReplay();
        emit replayFinished();
    }
}

bool Replayer::loadFile(const QString &filePath)
{
    cleanup();
    
    replayFile = new QFile(filePath, this);
    
    if (!replayFile->open(QIODevice::ReadOnly)) {
        emit errorOccurred(QString("Could not open file: %1").arg(filePath));
        delete replayFile;
        replayFile = nullptr;
        return false;
    }

    messages.clear();
    
    // Read all messages from file
    QByteArray buffer;
    qint64 timestamp;
    
    while (!replayFile->atEnd()) {
        // Read timestamp (8 bytes, big-endian)
        buffer = replayFile->read(sizeof(qint64));
        if (buffer.size() != sizeof(qint64)) {
            if (!replayFile->atEnd()) {
                qWarning() << "Incomplete timestamp in replay file";
            }
            break;
        }
        timestamp = qFromBigEndian<qint64>(buffer.data());
        
        // Read MAVLink message
        mavlink_message_t message_m;
        mavlink_status_t parser_status;
        bool messageComplete = false;
        
        while (!replayFile->atEnd() && !messageComplete) {
            buffer = replayFile->read(1);
            if (buffer.isEmpty())
                break;
                
            if (mavlink_parse_char(MAVLINK_COMM_1, (uint8_t)buffer[0], &message_m, &parser_status)) {
                messages.append({timestamp, message_m});
                messageComplete = true;
            }
        }
        
        if (!messageComplete) {
            qWarning() << "Incomplete message in replay file at position" << replayFile->pos();
            break;
        }
    }
    
    if (messages.isEmpty()) {
        emit errorOccurred("No valid messages found in file");
        delete replayFile;
        replayFile = nullptr;
        return false;
    }
    
    _currentFile = filePath;
    qInfo() << "Loaded" << messages.size() << "messages from" << filePath;
    
    return true;
}

void Replayer::cleanup()
{
    if (replayFile) {
        replayFile->close();
        delete replayFile;
        replayFile = nullptr;
    }
    messages.clear();
}
