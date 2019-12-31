import QtQuick 2.11

Rectangle {
    id: root

    property alias label: text.text
    property alias background: root.color
    property alias foreground: text.color
    property bool enabled: true
    signal clicked

    color: "#333333"
    border.width: 2
    border.color: text.color

    implicitWidth: text.width + 20
    implicitHeight: text.height + 20

    Text {
        id: text
        color: "white"
        font.pixelSize: 20
        anchors.centerIn: parent

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (root.enabled)
                    root.clicked()
            }
        }
    }
}
