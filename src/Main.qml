import QtQuick
import QtQuick.Controls.Fusion

ApplicationWindow {
    width: 800
    height: 600
    visible: true
    title: qsTr("MARSH Manager")

    menuBar: MenuBar {
        HelpMenu {}
    }

    HorizontalHeaderView {
        id: horizontalHeader
        anchors.left: treeView.left
        anchors.top: parent.top
        syncView: treeView
        clip: true
    }

    TreeView {
        id: treeView
        anchors.left: parent.left
        anchors.top: horizontalHeader.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true

        model: appData.networkDisplay.model

        delegate: TreeViewDelegate {
            onClicked: appData.networkDisplay.itemClicked(treeView.index(
                                                              row, column))

            TapHandler {
                acceptedModifiers: Qt.ControlModifier
                onTapped: {
                    if (treeView.isExpanded(row))
                        treeView.collapseRecursively(row)
                    else
                        treeView.expandRecursively(row)
                }
            }
        }
    }
}
