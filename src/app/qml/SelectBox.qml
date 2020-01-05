import QtQuick 2.11

Rectangle {
    id: root

    property color unselectedColor: "black"
    property color selectedColor: "#333333"
    property color foregroundColor: "#cccccc"

    property bool selected: false

    color: backgroundColor
    border.width: 2
    border.color: foregroundColor

    Text {
        visible: root.selected
        anchors.centerIn: parent
        text: "x"
        font.pixelSize: root.width / 2
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.selected = !root.selected
    }
}
