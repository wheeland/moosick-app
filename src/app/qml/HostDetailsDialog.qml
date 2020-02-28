import QtQuick 2.11

import Moosick 1.0 as Moosick

Rectangle {
    id: root

    property Moosick.HttpClient httpClient: _app.httpClient

    Connections {
        target: httpClient
        onHostValidChanged: {
            if (!httpClient.hostValid) {
                root.visible = true;
                hostEntry.text = httpClient.host;
                portEntry.text = httpClient.port;
            }
        }
    }

    color: "black"
    property real fontSize: 20

    border.color: "white"
    border.width: 3

    function removeFocus() {
        hostEntry.hasInputFocus = false;
        portEntry.hasInputFocus = false;
        userEntry.hasInputFocus = false;
        passEntry.hasInputFocus = false;
    }

    SimpleButton {
        id: cancelButton
        label: "Cancel"
        width: parent.width * 0.22
        x: 0.2 * parent.width - 0.5 * width
        y: parent.width / 16
        onClicked: {
            removeFocus();
            root.visible = false
        }
    }

    SimpleButton {
        id: okButton
        label: "OK"
        width: parent.width * 0.22
        x: 0.8 * parent.width - 0.5 * width
        y: parent.width / 16
        onClicked: {
            removeFocus();
            httpClient.host = hostEntry.text;
            httpClient.port = portEntry.text;
            root.visible = false;
        }
    }

    Column {
        spacing: 20
        anchors {
            left: parent.left
            right: parent.right
            top: okButton.bottom
            bottom: parent.bottom
            margins: 20
        }

        HostDetailsDialogEntry {
            id: hostEntry
            label: "Host"
            inputMethodHints: Qt.ImhNoPredictiveText
        }

        HostDetailsDialogEntry {
            id: portEntry
            label: "Port"
            inputMethodHints: Qt.ImhDigitsOnly | Qt.ImhNoPredictiveText
        }

        HostDetailsDialogEntry {
            id: userEntry
            label: "User"
            inputMethodHints: Qt.ImhNoPredictiveText
        }

        HostDetailsDialogEntry {
            id: passEntry
            label: "Pass"
            echoMode: TextInput.Password
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhHiddenText
        }
    }
}
