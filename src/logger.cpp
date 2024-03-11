#include "logger.h"
#include <QFileDialog>
#include <QtEndian>
#include "applicationdata.h"
#include "mavlink/all/mavlink.h" // IWYU pragma: keep; always include the mavlink.h file for selected dialect

Logger::Logger(QObject *parent)
    : QObject{parent}
{
    _outputDir = QDir::current();
    _outputDir.cd("marsh-logs");
}

void Logger::setAppData(ApplicationData *appData)
{
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
    return savingNow() ? outputFile->fileName() : _outputDir.filePath("<datetime>.tlog");
}

void Logger::setSavingNow(bool saving)
{
    if (savingNow() == saving)
        return; // do nothing on repeated calls

    if (saving) {
        if (!_outputDir.exists()) {
            Q_ASSERT(_outputDir.mkpath("."));
        }

        outputFile = new QFile(_outputDir.filePath("save.tlog"), this);

        Q_ASSERT(outputFile->open(QIODevice::ReadWrite));
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
