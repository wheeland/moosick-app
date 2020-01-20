import QtQuick 2.11

import Moosick 1.0

Rectangle {
    id: root

    property DbAlbum album
    property bool expanded: false

    property color textColor: "white"
    property color backgroundColor: "#444444"
    property color songColor: "#111111"

    height: tagSpace.height + column.height + 10
    color: root.backgroundColor
    clip: true

    Behavior on height {
        NumberAnimation {
            duration: slideDuration
        }
    }

    Text {
        id: albumNameText
        x: 10
        anchors.verticalCenter: tagSpace.verticalCenter
        color: root.textColor
        text: root.album.name
        font.bold: true
        font.pixelSize: textSize
    }

    LibraryTagSpace {
        id: tagSpace
        model: root.album.tags
        anchors {
            left: albumNameText.right
            leftMargin: 50
            top: parent.top
            right: durationText.left
            margins: 5
        }
        height: Math.max(childrenRect.height, albumNameText.height)
    }

    Text {
        id: durationText
        anchors {
            right: parent.right
            rightMargin: 10
            verticalCenter: tagSpace.verticalCenter
        }

        color: root.textColor
        text: root.album.durationString
        font.italic: true
        font.pixelSize: textSize
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.expanded = !root.expanded
    }

    Column {
        id: column
        height: (childrenRect.height > 0) ? (childrenRect.height + 10) : 0

        anchors {
            top: tagSpace.bottom
            topMargin: 5
            left: parent.left
            leftMargin: 5
            right: parent.right
            rightMargin: 5
        }

        spacing: 5

        Repeater {
            model: root.expanded ? root.album.songs : null

            delegate: LibrarySong {
                width: parent.width
                song: model.song
            }
        }
    }
}