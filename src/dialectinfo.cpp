#include "dialectinfo.h"
#include <QDebug>
#include <QFile>

DialectInfo::DialectInfo(QObject *parent)
    : QObject{parent}
{
    _definitionFiles = QString(MESSAGE_DEFINITION_FILES).split(',');
}

void DialectInfo::loadDefinitions()
{
    loadedSections = 0;
    emit isReadyChanged(isReady());
    emit readyProgressChanged(readyProgress());

    for (const auto &basename : _definitionFiles) {
        QFile file(QString(":/msgdef/%1").arg(basename));
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open" << basename;
            continue;
        }
        QXmlStreamReader reader(&file);

        int sectionsFound = 0;

        // there is no error handling for malformed files, because they are put in this path
        // by a script for generating and validating MAVLink messages
        while (!reader.atEnd()) {
            if (reader.isStartElement() && reader.name() == QString("enums")) {
                sectionsFound++;
                loadEnums(reader);
                loadedSections++;
                emit readyProgressChanged(readyProgress());
            } else if (reader.isStartElement() && reader.name() == QString("messages")) {
                sectionsFound++;
                loadMessages(reader);
                loadedSections++;
                emit readyProgressChanged(readyProgress());
            }
            reader.readNext();
        }

        // match the section count even if some sections weren't present
        loadedSections += sectionsPerFile - sectionsFound;
        emit readyProgressChanged(readyProgress());
    }

    emit isReadyChanged(isReady());
}

std::optional<QString> DialectInfo::enumValueName(QString enumName, int value)
{
    if (!enums.contains(enumName) || !enums[enumName].entries.contains(value)) {
        return std::nullopt;
    }
    return enums[enumName].entries[value].name;
}

std::optional<QString> DialectInfo::componentName(ComponentId id)
{
    auto marshName = enumValueName("MARSH_COMPONENT", id.value());
    if (marshName) {
        return marshName->mid(QString("MARSH_COMP_ID_").size());
    }
    auto mavName = enumValueName("MAV_COMPONENT", id.value());
    if (mavName) {
        return mavName->mid(QString("MAV_COMP_ID_").size());
    }
    return std::nullopt;
}

QStringList DialectInfo::definitionFiles() const
{
    return _definitionFiles;
}

bool DialectInfo::isReady() const
{
    return loadedSections == _definitionFiles.size() * sectionsPerFile;
}

float DialectInfo::readyProgress() const
{
    return static_cast<float>(loadedSections) / (_definitionFiles.size() * sectionsPerFile);
}

void DialectInfo::loadEnums(QXmlStreamReader &r)
{
    while (!(r.isEndElement() && r.name() == QString("enums"))) {
        if (!(r.isStartElement() && r.name() == QString("enum"))) {
            r.readNext();
            continue;
        }

        const auto enumName = r.attributes().value("name").toString();
        if (!enums.contains(enumName)) {
            enums[enumName] = EnumInfo{};
        }
        auto &enumInfo = enums[enumName];

        if (r.attributes().value("bitmask") == QString("true")) {
            enumInfo.bitmask = true;
        }

        while (!(r.isEndElement() && r.name() == QString("enum"))) {
            if (r.isStartElement() && r.name() == QString("description")) {
                enumInfo.description = r.readElementText();
            } else if (r.isStartElement() && r.name() == QString("deprecated")) {
                enumInfo.deprecated = true;
                r.readNext();
            } else if (r.isStartElement() && r.name() == QString("wip")) {
                enumInfo.wip = true;
                r.readNext();
            } else if (r.isStartElement() && r.name() == QString("entry")) {
                const auto name = r.attributes().value("name").toString();
                const auto value = r.attributes().value("value").toInt();
                enumInfo.entries[value] = EntryInfo{name, ""};
                auto &entryInfo = enumInfo.entries[value];

                while (!(r.isEndElement() && r.name() == QString("entry"))) {
                    if (r.isStartElement() && r.name() == QString("description")) {
                        entryInfo.description = r.readElementText();
                    } else if (r.isStartElement() && r.name() == QString("param")) {
                        const auto index = r.attributes().value("index").toInt();
                        const auto text = r.readElementText();
                        entryInfo.params[index] = text;
                    } else
                        r.readNext();
                }
            } else
                r.readNext();
        }
        r.readNext();
    }
}

void DialectInfo::loadMessages(QXmlStreamReader &r)
{
    while (!r.isEndElement() && r.name() == QString("messages")) {
        // TODO: load message details
        r.readNext();
    }
}
