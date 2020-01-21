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
        anchors.fill: parent
    }

    PlayerBar {
        id: player
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    KeyboardItem {
        id: keyboard
        width: parent.width
        y: Qt.inputMethod.visible ? parent.height - height : parent.height
        visible: (y < parent.height)
        Behavior on y { NumberAnimation { duration: 100 } }
    }
}
