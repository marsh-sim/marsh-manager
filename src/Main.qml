import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts

ApplicationWindow {
    id: rootWindow
    width: 800
    height: 600
    visible: true
    title: "MARSH Manager v" + Qt.application.version

    menuBar: MenuBar {
        HelpMenu {}
    }

    function formatDuration(milliseconds) {
        const ms = (milliseconds % 1000).toString()
        const s = (Math.floor(milliseconds / 1000) % 60).toString()
        const m = (Math.floor(milliseconds / 1000 / 60)).toString()
        return `${m}:${s.padStart(2, '0')}.${ms.padStart(3, '0')}`
    }

    function formatSize(bytes) {
        const prefix = ['', 'Ki', 'Mi', 'Gi']
        let order = 0
        while (bytes > 1024) {
            bytes /= 1024
            order += 1
        }
        return bytes.toString().slice(0, 4) + prefix[order] + 'B'
    }

    Column {
        id: statusBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 10

        property real saveStartTime: Number.NaN

        RowLayout {
            Text {
                text: qsTr("Listening on port ") + appData.router.listenPort
                color: palette.text
            }

            Text {
                id: currentTime
                color: palette.text

                Connections {
                    target: rootWindow
                    function onBeforeRendering() {
                        let text = qsTr("Current time: ")

                        const time_us = (new Date).getTime() * 1000
                        text += appData.networkDisplay.formatUpdateTime(time_us)

                        if (!isNaN(statusBar.saveStartTime)) {
                            text += qsTr(", saving log for: ")
                            text += formatDuration(
                                        time_us / 1000 - statusBar.saveStartTime)
                        }

                        currentTime.text = text
                    }
                }
            }
        }

        RowLayout {
            spacing: 3
            Text {
                text: {
                    if (appData.logger.savingNow) {
                        const size = formatSize(appData.logger.bytesWritten)
                        return size + qsTr(" written to")
                    } else {
                        return qsTr("Will save to")
                    }
                }
                color: palette.text
            }
            Text {
                text: appData.logger.outputPath
                color: palette.text
            }
        }

        Flow {
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 10

            Button {
                text: qsTr("Save data")
                enabled: !appData.logger.savingNow
                onClicked: appData.logger.savingNow = true
            }

            Button {
                text: qsTr("Stop saving")
                enabled: appData.logger.savingNow
                onClicked: appData.logger.savingNow = false
            }

            Button {
                text: qsTr("Choose save directory")
                enabled: !appData.logger.savingNow
                onClicked: {
                    appData.logger.outputDir = appData.logger.getDirectoryWithDialog()
                }
            }

            TextField {
                placeholderText: qsTr("File comment")
                maximumLength: 50 // maximum for STATUSTEXT message
                onTextChanged: appData.logger.fileComment = text
                enabled: !appData.logger.savingNow
            }

            CheckBox {
                id: saveLimitedTime
                text: qsTr("Save limited time")
                enabled: !appData.logger.savingNow
            }
        }

        Flow {
            visible: saveLimitedTime.checked

            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 10

            Text {
                text: qsTr("Save time (in seconds):")
                enabled: !appData.logger.savingNow
                color: palette.text
            }

            TextField {
                id: saveTimeSeconds
                text: "60"
                enabled: !appData.logger.savingNow
                validator: IntValidator {
                    bottom: 0
                }
            }

            Timer {
                id: saveLimitTimer
                interval: parseInt(saveTimeSeconds.text) * 1000
                onTriggered: appData.logger.savingNow = false
            }
        }

        Connections {
            target: appData.logger
            function onSavingNowChanged(savingNow) {
                if (savingNow) {
                    if (saveLimitedTime.checked) {
                        saveLimitTimer.start()
                    }
                    statusBar.saveStartTime = (new Date).getTime()
                } else {
                    saveLimitTimer.stop()
                    statusBar.saveStartTime = Number.NaN
                }
            }
        }
    }

    NetworkView {
        anchors.top: statusBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 10
    }
}
