import QtQuick 2.11

Rectangle {
    id: root

    color: "black"
    property real fontSize: 20

    visible: _app.database.confirmationVisible

    border.color: "white"
    border.width: 3

    implicitWidth: 400
    implicitHeight: cancelButton.y + cancelButton.height + 20

    Text {
        id: labelText
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 20
        }
        text: _app.database.confirmationText
        font.pixelSize: root.fontSize * 1.5
        wrapMode: Text.WordWrap
        color: "white"
        horizontalAlignment: Text.AlignHCenter
    }

    SimpleButton {
        id: cancelButton
        label: "Cancel"
        width: parent.width * 0.22
        x: 0.2 * parent.width - 0.5 * width
        anchors {
            top: labelText.bottom
            topMargin: 20
        }

        onClicked: _app.database.confirm(false)
    }

    SimpleButton {
        id: okButton
        label: "OK"
        width: parent.width * 0.22
        x: 0.8 * parent.width - 0.5 * width
        anchors {
            top: labelText.bottom
            topMargin: 20
        }
        onClicked: _app.database.confirm(true)
    }
}
