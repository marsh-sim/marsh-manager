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

    ColumnLayout {
        id: statusBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        spacing: 10

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
                        const timestamp = (new Date).getTime() * 1000
                        const text = appData.networkDisplay.formatUpdateTime(
                                       timestamp)
                        currentTime.text = qsTr("Current time: ") + text
                    }
                }
            }
        }

        RowLayout {
            spacing: 3
            Text {
                text: appData.logger.savingNow ? qsTr("Saving to") : qsTr(
                                                     "Will save to")
                color: palette.text
            }
            Text {
                text: appData.logger.outputPath
                color: palette.text
            }
        }

        RowLayout {
            Button {
                text: qsTr("Save data")
                enabled: !appData.logger.savingNow
                onClicked: {
                    appData.logger.savingNow = true
                    if (saveLimitedTime.checked) {
                        saveLimitTimer.start()
                    }
                }
            }

            Button {
                text: qsTr("Stop saving")
                enabled: appData.logger.savingNow
                onClicked: {
                    appData.logger.savingNow = false
                    saveLimitTimer.stop()
                }
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

        RowLayout {
            visible: saveLimitedTime.checked

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
    }

    NetworkView {
        anchors.top: statusBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 10
    }
}
