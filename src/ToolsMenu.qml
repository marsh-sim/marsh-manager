import QtQuick
import QtQuick.Controls.Fusion
import QtGraphs

Menu {
    id: root
    title: qsTr("Tools")

    MenuItem {
        text: plotWindow.title
        onTriggered: plotWindow.visible = true
    }

    Window {
        width: 800
        height: 600
        id: plotWindow
        title: qsTr("Plot window")
        GraphsView {
            anchors.fill: parent
            axisX: ValueAxis {
                max: 5
                tickInterval: 1
                subTickCount: 9
                labelDecimals: 1
            }
            axisY: ValueAxis {
                max: 10
                tickInterval: 1
                subTickCount: 4
                labelDecimals: 1
            }

            LineSeries {
                id: lineSeries1
                width: 4
                XYPoint { x: 0; y: 0 }
                XYPoint { x: 1; y: 2.1 }
                XYPoint { x: 2; y: 3.3 }
                XYPoint { x: 3; y: 2.1 }
                XYPoint { x: 4; y: 4.9 }
                XYPoint { x: 5; y: 3.0 }
            }

            LineSeries {
                id: lineSeries2
                width: 4
                XYPoint { x: 0; y: 5.0 }
                XYPoint { x: 1; y: 3.3 }
                XYPoint { x: 2; y: 7.1 }
                XYPoint { x: 3; y: 7.5 }
                XYPoint { x: 4; y: 6.1 }
                XYPoint { x: 5; y: 3.2 }
            }
        }
    }
}
