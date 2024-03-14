import QtQuick
import QtQuick.Controls.Fusion
import QtQuick.Dialogs

Menu {
    id: root
    title: qsTr("Help")

    readonly property url helpUrl: Qt.url(
                                       "https://marsh-sim.github.io/manager/")

    MenuItem {
        text: qsTr("Online documentation")
        onTriggered: {
            if (!Qt.openUrlExternally(helpUrl)) {
                onlineHelpFail.visible = true
            }
        }
    }

    MenuItem {
        text: qsTr("About MARSH Manager")
        onTriggered: appInfo.visible = true
    }

    MenuItem {
        text: qsTr("Open-source components")
        onTriggered: componentsInfo.visible = true
    }

    MenuItem {
        text: gplInfo.title
        onTriggered: gplInfo.visible = true
    }

    MessageDialog {
        id: onlineHelpFail
        buttons: MessageDialog.Ok
        text: qsTr("Failed to open help page in system browser")
        informativeText: helpUrl + "\n." // FIXME: last line of text partially hidden
    }

    MessageDialog {
        id: appInfo
        buttons: MessageDialog.Ok
        text: `MARSH Manager\nVersion ${Qt.application.version}\nBuild type ${appData.buildType}\nGit commit no. ${appData.buildGitCommitCount} ref ${appData.buildGitHash}\n\nCopyright © 2024 Marek S. Łukasiewicz`
        informativeText: qsTr("
Developed at Department of Aerospace Science and Technology (DAER) of Politecnico di Milano

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

A copy of the GNU General Public License is embedded in this program. Alternatively, see <https://www.gnu.org/licenses/>
.")
    }

    MessageDialog {
        id: componentsInfo
        buttons: MessageDialog.Ok
        text: qsTr("MARSH Manager includes the following open source components:")
        informativeText: qsTr("
The Qt Framework, https://qt.io/ (GPLv3)
MAVLink message definitons, https://mavlink.io/ (MIT)
.")
    }

    Window {
        width: 600
        height: 600
        id: gplInfo
        title: qsTr("GNU GPLv3 License")
        ScrollView {
            anchors.fill: parent
            Text {
                id: licenseText
                font.pointSize: 12
                wrapMode: Text.NoWrap
                text: appData.licenseText
            }
        }
    }
}
