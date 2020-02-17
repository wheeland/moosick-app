import QtQuick 2.11
import QtQuick.Controls 2.4

import Moosick 1.0 as Moosick

import QtQml 2.11 as QQML
import QtMultimedia 5.11 as QQM
import QtQuick.Layouts 1.11 as QQL

Rectangle {
    id: root

    color: "black"

    border.color: "white"
    border.width: 3

    SwipeHeader {
        id: header
        entries: ["Search", "Library", "Playlist" ]
        index: swipeView.currentIndex
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 5
        }

        height: 50
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

    Row {
        height: childrenRect.height
        anchors.left: parent.left
        anchors.bottom: player.top
        anchors.margins: 20
        spacing: 20

        SimpleButton {
            visible: _app.database.changesPending
            fontSize: 10
            label: "Changing..."
        }

        SimpleButton {
            visible: _app.database.isSyncing
            fontSize: 10
            label: "Syncing..."
        }

        SimpleButton {
            visible: _app.database.downloadsPending
            fontSize: 10
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
        width: 20
        height: 20
        property var colors: [ "black", "#333333", "#666666" ]
        color: colors[logger.state]
        onClicked: logger.state = (logger.state + 1) % 3
    }
}
