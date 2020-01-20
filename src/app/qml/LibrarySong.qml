import QtQuick 2.11

import Moosick 1.0

Rectangle {
    id: root

    property DbSong song
    property color textColor: "white"
    property color backgroundColor: "#111111"

    height: Math.max(songNameText.height, tagSpace.height) + 10
    color: root.backgroundColor

    Text {
        id: positionText
        anchors.verticalCenter: parent.verticalCenter
        x: 5
        width: 1.5 * textSize
        color: root.textColor
        text: root.song.position
        font.pixelSize: textSize
        horizontalAlignment: Text.AlignRight
    }

    Text {
        id: songNameText
        anchors.verticalCenter: parent.verticalCenter
        x: 50
        color: root.textColor
        text: root.song.name
        font.bold: true
        font.pixelSize: textSize
    }

    LibraryTagSpace {
        id: tagSpace
        model: root.song.tags
        anchors {
            left: songNameText.right
            leftMargin: 10
            top: parent.top
            right: durationText.left
            rightMargin: 10
            margins: 5
        }
        height: childrenRect.height
    }

    Text {
        id: durationText
        anchors {
            right: parent.right
            rightMargin: 10
            verticalCenter: parent.verticalCenter
        }
        color: root.textColor
        text: root.song.durationString
        font.italic: true
        font.pixelSize: textSize
    }
}
