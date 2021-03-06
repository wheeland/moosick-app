import QtQuick 2.11
import QtQuick.Controls 2.4

import Moosick 1.0 as Moosick

Rectangle {
    id: root

    color: "black"

    border.color: "white"
    border.width: 3

    SwipeHeader {
        id: header
        entries: ["Search", "Library", "Playlist", "Tags" ]
        index: swipeView.currentIndex
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 5
        }

        offset: 10
        onChange: swipeView.currentIndex = toIndex
    }

    SwipeView {
        id: swipeView
        clip: true

        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: player.top
            margins: 10
        }

        SearchPane {
        }

        LibraryPane {
        }

        PlaylistPane {
        }

        TagsPane {
        }
    }

    MultiChoicePopup {
        id: multiChoicePopup
        anchors.fill: parent
    }

    InputBlocker {
        visible: stringChoice.visible || confirmationDialog.visible
    }

    ConfirmationDialog {
        id: confirmationDialog
        anchors.centerIn: parent
    }

    LibraryItemEditPopup {
        id: stringChoice
        visible: _app.database.editItemVisible
        stringsModel: _app.database.editStringList
        tagsModel: _app.database.tagsModel
        anchors.centerIn: parent
        width: parent.width * 0.8
        height: parent.height * 0.4
        onOkClicked: _app.database.editOkClicked()
        onCancelClicked: _app.database.editCancelClicked()
    }

    HostDetailsDialog {
        id: hostDetailsDialog
        anchors.centerIn: parent
        width: parent.width * 0.8
        height: parent.height * 0.4
    }

    InputBlocker {
        visible: _app.httpClient.hasPendingSslError
    }

    PendingSslErrorPopup {
        anchors.centerIn: parent
        width: parent.width * 0.8
        height: parent.height * 0.4
    }

    Row {
        height: childrenRect.height
        anchors.left: parent.left
        anchors.bottom: player.top
        anchors.margins: 20
        spacing: 20

        SimpleButton {
            visible: _app.database.changesPending
            fontSize: _style.fontSizeIndicator
            label: "Changing..."
        }

        SimpleButton {
            visible: _app.database.isSyncing
            fontSize: _style.fontSizeIndicator
            label: "Syncing..."
        }

        SimpleButton {
            visible: _app.database.downloadsPending
            fontSize: _style.fontSizeIndicator
            label: "Downloading..."
        }
    }

    PlayerBar {
        id: player
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    LogOutput {
        id: logger
        property int state: 0
        enabled: (state == 2)
        visible: (state > 0)
        width: parent.width
        height: parent.height / 2
    }

    Loader {
        visible: !_isAndroid && (y < parent.height)
        active: !_isAndroid
        source: "KeyboardItem.qml"

        width: parent.width
        y: Qt.inputMethod.visible ? parent.height - height : parent.height
        Behavior on y { NumberAnimation { duration: 100 } }
    }

    SimpleButton {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 10
        width: 40
        height: 40
        property var colors: [ "black", "#333333", "#666666" ]
        color: colors[logger.state]
        onClicked: logger.state = (logger.state + 1) % 3
    }
}
