import QtQuick 2.11

import Moosick 1.0 as Moosick

Rectangle {
    id: root

    property color backgroundColor: "black"
    property color foregroundColor: "#888888"
    property color textColor: "black"
    property color activeColor: "#cccccc"
    property real pixelSize: 18

    color: root.backgroundColor

    property real offset

    property var entries: []
    property int index

    signal change(var toIndex)

    height: inner.height + 2 * offset

    Item {
        id: inner
        anchors.centerIn: parent
        width: root.width - 2 * root.offset
        height: childrenRect.height

        property real entryWidth: (inner.width - (root.entries.length - 1) * root.offset) / root.entries.length

        Repeater {
            model: entries.length

            delegate: Rectangle {
                x: (width + root.offset) * index
                width: inner.entryWidth
                height: text.height + 10

                color: (index === root.index) ? root.activeColor : root.foregroundColor

                Text {
                    id: text
                    anchors.centerIn: parent
                    text: root.entries[index]
                    color: root.textColor
                    font.pixelSize: root.pixelSize
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.change(index)
                }
            }
        }
    }
}
