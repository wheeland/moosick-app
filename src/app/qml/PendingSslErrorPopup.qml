import QtQuick 2.11

import Moosick 1.0 as Moosick

Rectangle {
    id: root

    property real fontSize: 20

    visible: _app.httpClient.hasPendingSslError

    color: "black"
    border.color: "white"
    border.width: 3

    SimpleButton {
        id: cancelButton
        label: "Cancel"
        width: parent.width * 0.22
        x: 0.2 * parent.width - 0.5 * width
        y: parent.width / 16
        onClicked: {
            _app.httpClient.ignorePendingSslError(false);
        }
    }

    SimpleButton {
        id: okButton
        label: "Ignore"
        width: parent.width * 0.22
        x: 0.8 * parent.width - 0.5 * width
        y: parent.width / 16
        onClicked: {
            _app.httpClient.ignorePendingSslError(true);
        }
    }

    Flickable {
        anchors {
            left: parent.left
            right: parent.right
            top: okButton.bottom
            bottom: parent.bottom
            margins: 20
        }
        clip: true
        contentWidth: width
        contentHeight: errorText.height

        Text {
            id: errorText
            width: parent.width

            text: _app.httpClient.pendingSslError
            color: "white"
            font.pixelSize: 0.7 * root.fontSize
            wrapMode: Text.Wrap
        }
    }
}
