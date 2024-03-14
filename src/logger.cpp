#include "logger.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QtEndian>
#include "applicationdata.h"
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect

Logger::Logger(QObject *parent)
    : QObject{parent}
{
    _outputDir = QDir::home();
    const QString dirName = "marsh-logs";
    if (!_outputDir.exists(dirName) && !_outputDir.mkdir(dirName)) {
        QMessageBox msgBox;
        msgBox.setText("Could not create output directory");
        msgBox.exec();
    }
    _outputDir.cd(dirName);
}

void Logger::setAppData(ApplicationData *appData)
{
    this->appData = appData;
    connect(appData->router(), &Router::messageReceived, this, &Logger::receiveMessage);
}

QString Logger::getDirectoryWithDialog()
{
    return QFileDialog::getExistingDirectory(nullptr, "Select log output directory", outputDir());
}

bool Logger::savingNow() const
{
    return outputFile != nullptr;
}

QString Logger::outputDir() const
{
    return _outputDir.absolutePath();
}

QString Logger::outputPath() const
{
    return savingNow() ? outputFile->fileName() : _outputDir.filePath(formatFilename());
}

void Logger::setSavingNow(bool saving)
{
    if (savingNow() == saving)
        return; // do nothing on repeated calls

    if (saving) {
        const auto dateTime = QDateTime::fromMSecsSinceEpoch(Message::currentTime() / 1000);
        outputFile = new QFile(_outputDir.filePath(formatFilename(dateTime)), this);

        Q_ASSERT(outputFile->open(QIODevice::ReadWrite));

        if (_fileComment.size() > 0) {
            // write the file comment as the very first message
            mavlink_statustext_t statustext;
            statustext.severity = MAV_SEVERITY_INFO;

            std::fill(statustext.text, statustext.text + sizeof(statustext.text), 0);
            auto textData = _fileComment.toUtf8();
            qstrncpy(statustext.text, textData, sizeof(statustext.text));

            statustext.id = 0;
            statustext.chunk_seq = 0;

            mavlink_message_t message_m;
            mavlink_msg_statustext_encode(appData->localSystemId(),
                                          appData->localComponentId(),
                                          &message_m,
                                          &statustext);

            receiveMessage({dateTime.toMSecsSinceEpoch() * 1000, message_m});
        }
    } else {
        outputFile->close();
        delete outputFile;
        outputFile = nullptr;
    }

    emit savingNowChanged(savingNow());
    emit outputPathChanged(outputPath());
}

void Logger::setOutputDir(QString dir)
{
    _outputDir = QDir::fromNativeSeparators(dir);

    emit outputDirChanged(outputDir());
    emit outputPathChanged(outputPath());
}

void Logger::receiveMessage(Message message)
{
    if (!savingNow())
        return;

    QByteArray write_buffer(MAVLINK_MAX_PACKET_LEN, Qt::Initialization::Uninitialized);

    // write timestamp as big-endian number
    qToBigEndian(message.timestamp, write_buffer.data());
    outputFile->write(write_buffer, sizeof(message.timestamp));

    const auto length = mavlink_msg_to_send_buffer((quint8 *) write_buffer.data(), &message.m);
    outputFile->write(write_buffer, length);
}

QString Logger::formatFilename(std::optional<QDateTime> datetime) const
{
    QString datePart = datetime ? QString("%1%2%3T%4%5%6")
                                      .arg(datetime->date().year(), 4, 10, QChar('0'))
                                      .arg(datetime->date().month(), 2, 10, QChar('0'))
                                      .arg(datetime->date().day(), 2, 10, QChar('0'))
                                      .arg(datetime->time().hour(), 2, 10, QChar('0'))
                                      .arg(datetime->time().minute(), 2, 10, QChar('0'))
                                      .arg(datetime->time().second(), 2, 10, QChar('0'))
                                : "<datetime>";
    QString commentPart = _fileComment.size() > 0 ? "_" + _fileComment : "";
    return datePart + commentPart + ".tlog";
}

QString Logger::fileComment() const
{
    return _fileComment;
}

void Logger::setFileComment(const QString &newFileComment)
{
    if (_fileComment == newFileComment)
        return;
    _fileComment = newFileComment;
    emit fileCommentChanged(_fileComment);
    if (!savingNow()) {
        emit outputPathChanged(outputPath());
    }
}
