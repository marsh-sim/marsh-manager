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

    RowLayout {
        id: statusBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 10
        spacing: 10

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

    NetworkView {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: statusBar.bottom
        anchors.topMargin: 10
    }
}
