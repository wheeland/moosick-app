import QtQuick 2.11

import Moosick 1.0 as Moosick

Item {
    id: root

    property alias label: labelText.text
    property alias text: lineEdit.text
    property alias hasInputFocus: lineEdit.hasInputFocus
    property alias inputMethodHints: lineEdit.inputMethodHints
    property alias echoMode: lineEdit.echoMode
    property real fontSize: 20

    property real editX: labelText.x + labelText.width + 20

    width: parent.width
    height: lineEdit.height

    Text {
        id: labelText
        font.pixelSize: root.fontSize
        color: "white"
    }

    LineEdit {
        id: lineEdit
        background: hasInputFocus ? "#333333" : "black"
        foreground: "white"
        pixelSize: root.fontSize
        anchors.left: parent.left
        anchors.leftMargin: root.editX
        anchors.right: parent.right
    }
}
