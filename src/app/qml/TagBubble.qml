import QtQuick 2.11

import Moosick 1.0 as Moosick

Rectangle {
    id: root

    property Moosick.DbTag tag

    property color backgroundColor: "#aa9977"
    property color foregroundColor: "#000000"
    property color borderColor: "#ffffff"
    property real size: 10.0

    color: root.borderColor

    width: text.width + 4 + 1.2 * root.size
    height: text.height + 4 + 0.5 * root.size
    radius: 0.5 * height

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: 0.5 * height
        color: root.backgroundColor

        Text {
            id: text
            anchors.centerIn: parent
            text: root.tag.name
            color: root.foregroundColor
            font.pixelSize: root.size
        }
    }
}
