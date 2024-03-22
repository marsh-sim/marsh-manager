#ifndef DIALECTINFO_H
#define DIALECTINFO_H

#include <QMap>
#include <QObject>
#include <QXmlStreamReader>
#include "message.h"
#include <optional>

class DialectInfo : public QObject
{
    Q_OBJECT
public:
    explicit DialectInfo(QObject *parent = nullptr);

    Q_PROPERTY(QStringList definitionFiles READ definitionFiles CONSTANT)
    Q_PROPERTY(bool isReady READ isReady NOTIFY isReadyChanged FINAL)
    Q_PROPERTY(float readyProgress READ readyProgress NOTIFY readyProgressChanged FINAL)

    /// parse XML message definitions to build a database of names and descriptions
    Q_INVOKABLE void loadDefinitions();
    Q_INVOKABLE std::optional<QString> enumValueName(QString enumName, int value);
    Q_INVOKABLE std::optional<QString> componentName(ComponentId id);

    QStringList definitionFiles() const;
    bool isReady() const;
    float readyProgress() const;

signals:
    void isReadyChanged(bool ready);
    void readyProgressChanged(float progress);

private:
    struct EntryInfo
    {
        QString name;
        QString description;
        QMap<int, QString> params;
    };

    struct EnumInfo
    {
        /// indexed by entry value
        QMap<int, EntryInfo> entries;
        QString description;
        bool bitmask;
        bool deprecated;
        bool wip;
    };

    void loadEnums(QXmlStreamReader &reader);
    void loadMessages(QXmlStreamReader &reader);

    QStringList _definitionFiles;
    const int sectionsPerFile = 2; // enums, messages
    int loadedSections = 0;

    /// indexed by enum name
    QMap<QString, EnumInfo> enums;
};

#endif // DIALECTINFO_H
