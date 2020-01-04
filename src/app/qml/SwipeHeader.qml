import QtQuick 2.11

import Moosick 1.0 as Moosick

Rectangle {
    id: root

    property color backgroundColor: "black"
    property color foregroundColor: "#888888"
    property color textColor: "black"
    property color activeColor: "#cccccc"

    color: root.backgroundColor

    property real offset

    property var entries: []
    property int index

    signal change(var toIndex)

    Item {
        id: inner
        anchors.fill: parent
        anchors.margins: root.offset

        property real entryWidth: (inner.width - (root.entries.length - 1) * root.offset) / root.entries.length

        Repeater {
            model: entries.length

            delegate: Rectangle {
                x: (width + root.offset) * index
                width: inner.entryWidth
                height: inner.height

                color: (index === root.index) ? root.activeColor : root.foregroundColor

                Text {
                    anchors.centerIn: parent
                    text: root.entries[index]
                    color: root.textColor
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.change(index)
                }
            }
        }
    }
}
