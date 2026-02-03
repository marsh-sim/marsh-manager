import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts

ApplicationWindow {
    id: rootWindow
    width: 800
    height: 600
    visible: true
    title: "MARSH Manager v" + appData.version
           + (appData.buildType === "Release" ? "" : (" (" + appData.buildType + ")"))

    menuBar: MenuBar {
        ToolsMenu {}
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

        Text {
            text: qsTr("Listening on port %1 as system %2 component %3").arg(
                      appData.router.listenPort).arg(appData.localSystemId).arg(
                      appData.localComponentId)
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

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: palette.mid
            visible: replaySection.visible
        }

        Column {
            id: replaySection
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 10
            visible: true

            Text {
                text: qsTr("Replay Controls")
                font.bold: true
                color: palette.text
            }

            Text {
                text: qsTr("Selected file: ") + (appData.replayer.currentFile || qsTr("None"))
                color: palette.text
                wrapMode: Text.WrapAnywhere
                width: parent.width
            }

            Flow {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 10

                Button {
                    text: qsTr("Select replay file")
                    enabled: !appData.replayer.isReplaying
                    onClicked: {
                        const filePath = appData.replayer.selectFileWithDialog()
                        if (filePath && filePath.length > 0) {
                            // File selected but not started yet
                        }
                    }
                }

                Button {
                    text: qsTr("Start replay")
                    enabled: !appData.replayer.isReplaying && appData.replayer.currentFile.length > 0
                    onClicked: {
                        if (!appData.replayer.startReplay(appData.replayer.currentFile)) {
                            // Error is shown via errorOccurred signal
                        }
                    }
                }

                Button {
                    text: appData.replayer.isPaused ? qsTr("Resume") : qsTr("Pause")
                    enabled: appData.replayer.isReplaying
                    onClicked: {
                        if (appData.replayer.isPaused) {
                            appData.replayer.resumeReplay()
                        } else {
                            appData.replayer.pauseReplay()
                        }
                    }
                }

                Button {
                    text: qsTr("Stop replay")
                    enabled: appData.replayer.isReplaying
                    onClicked: appData.replayer.stopReplay()
                }

                Text {
                    text: qsTr("Speed:")
                    color: palette.text
                    anchors.verticalCenter: speedCombo.verticalCenter
                    enabled: appData.replayer.isReplaying
                }

                ComboBox {
                    id: speedCombo
                    model: ["0.5x", "1x", "2x", "5x"]
                    currentIndex: 1
                    enabled: appData.replayer.isReplaying
                    onActivated: {
                        const speeds = [0.5, 1.0, 2.0, 5.0]
                        appData.replayer.playbackSpeed = speeds[currentIndex]
                    }
                }
            }

            ProgressBar {
                anchors.left: parent.left
                anchors.right: parent.right
                visible: appData.replayer.isReplaying
                value: appData.replayer.progress
                
                Text {
                    anchors.centerIn: parent
                    text: qsTr("Replay: ") + Math.round(appData.replayer.progress * 100) + "%"
                    color: palette.text
                }
            }
        }

        Connections {
            target: appData.replayer
            function onErrorOccurred(error) {
                errorDialog.text = error
                errorDialog.open()
            }
            function onReplayFinished() {
                replayFinishedDialog.open()
            }
        }

        Dialog {
            id: errorDialog
            title: qsTr("Replay Error")
            property alias text: errorText.text
            modal: true
            standardButtons: Dialog.Ok
            anchors.centerIn: parent

            Text {
                id: errorText
                color: palette.text
            }
        }

        Dialog {
            id: replayFinishedDialog
            title: qsTr("Replay Finished")
            modal: true
            standardButtons: Dialog.Ok
            anchors.centerIn: parent

            Text {
                text: qsTr("Replay has finished successfully.")
                color: palette.text
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
