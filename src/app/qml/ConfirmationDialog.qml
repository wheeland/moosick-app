import QtQuick 2.11

Rectangle {
    id: root

    visible: _app.database.confirmationVisible

    color: "black"
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
        font.pixelSize: _style.fontSizeLabels
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
