import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts

Item {
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
            contentItem: Text {
                readonly property bool highlight: (model.editable ?? false)
                                                  && hovered

                text: (model.display
                       ?? "") + (highlight ? " (click to edit)" : "")
                color: model.decoration ?? palette.text
                font.bold: highlight
            }

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

        ScrollBar.horizontal: ScrollBar {}
        ScrollBar.vertical: ScrollBar {}
    }
}
