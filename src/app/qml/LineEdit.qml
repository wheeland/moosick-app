import QtQuick 2.11

Rectangle {
    id: root

    property alias background : root.color
    property alias foreground : input.color
    property alias text : input.text
    property real pixelSize: 11

    border.color: foreground
    border.width: 1

    TextInput {
        id: input
        font.pixelSize: root.pixelSize
        anchors.fill: parent
        anchors.margins: 5
    }
}
