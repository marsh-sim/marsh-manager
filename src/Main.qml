import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts

ApplicationWindow {
    id: rootWindow
    width: 800
    height: 600
    visible: true
    title: qsTr("MARSH Manager")

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
        }
    }

    NetworkView {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: statusBar.bottom
        anchors.topMargin: 10
    }
}
