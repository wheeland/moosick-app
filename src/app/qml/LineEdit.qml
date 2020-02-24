import QtQuick 2.11

Rectangle {
    id: root

    property alias background : root.color
    property alias foreground : input.color
    property alias text : input.text
    property alias displayText : input.displayText
    property real pixelSize: 11

    property alias hasInputFocus : input.focus
    property alias inputMethodHints : input.inputMethodHints
    property alias echoMode: input.echoMode

    border.color: foreground
    border.width: 1

    implicitHeight: input.implicitHeight + 10

    TextInput {
        id: input
        font.pixelSize: root.pixelSize
        anchors.fill: parent
        verticalAlignment: TextInput.AlignVCenter
        anchors.margins: 5
    }
}
