import QtQuick 2.11

Rectangle {
    id: root

    property alias label: text.text
    property color background: "#333333"
    property color backgroundClicked: Qt.lighter(background, 2.0)
    property alias foreground: text.color
    property bool enabled: true
    property alias fontSize : text.font.pixelSize
    signal clicked

    color: mouse.pressed ? backgroundClicked : background

    border.width: 2
    border.color: text.color

    implicitWidth: text.width + 20
    implicitHeight: text.height + 20

    Text {
        id: text
        color: "white"
        font.pixelSize: _style.fontSizeButtons
        anchors.centerIn: parent
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        onClicked: {
            if (root.enabled)
                root.clicked()
        }
    }
}
