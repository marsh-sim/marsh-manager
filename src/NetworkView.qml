import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import Qt.labs.qmlmodels

Item {
    Flow {
        id: buttonRow

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 10

        Button {
            text: qsTr("Hide timed out clients")
            onClicked: appData.displayModel.hideCurrentlyTimedOut()
        }

        Button {
            text: qsTr("Show %n hidden client(s)", "",
                       appData.displayModel.hiddenClientCount)
            enabled: appData.displayModel.hiddenClientCount
            onClicked: appData.displayModel.showAllClients()
        }
    }

    HorizontalHeaderView {
        id: horizontalHeader
        anchors.left: treeView.left
        anchors.top: buttonRow.bottom
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

        model: appData.displayModel
        delegate: chooser

        DelegateChooser {
            id: chooser
            role: "editable"
            DelegateChoice {
                roleValue: "true"
                TreeViewDelegate {
                    contentItem: Button {
                        contentItem: Text {
                            text: (model.display ?? "")
                            color: model.decoration ?? palette.buttonText
                            horizontalAlignment: (model.textAlign
                                                  ?? Text.Center)
                        }
                        onClicked: treeView.model.itemClicked(treeView.index(
                                                                  row, column))
                    }
                }
            }

            DelegateChoice {
                // default
                TreeViewDelegate {
                    contentItem: Text {
                        text: (model.display ?? "")
                        color: model.decoration ?? palette.text
                        horizontalAlignment: (model.textAlign ?? Text.AlignLeft)
                    }

                    onClicked: treeView.model.itemClicked(treeView.index(
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

        ScrollBar.horizontal: ScrollBar {}
        ScrollBar.vertical: ScrollBar {}
    }
}
