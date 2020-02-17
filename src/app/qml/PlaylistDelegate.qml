import QtQuick 2.11

import Moosick 1.0

Rectangle {
    id: root

    property PlaylistEntry entry

    color: "#333333"
    border.width: 2
    border.color: "white"

    height: 50

    QtObject {
        id: d
        readonly property real margin: 5
    }

    Image {
        id: image

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: d.margin

        source: entry.iconData
        fillMode: Image.PreserveAspectFit
    }

    Column {
        id: textColumn
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: image.right
        anchors.right: duration.left
        height: childrenRect.height

        Text {
            id: artist
            text: entry.artist
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: _style.fontSizeEntries
        }

        Item {
            id: albumTitle
            width: parent.width
            height: childrenRect.height
            Text {
                id: album
                text: entry.album
                x: parent.width / 4 - width / 2
                font.pixelSize: _style.fontSizeEntries
            }

            Text {
                id: title
                text: entry.title
                x: 3 * parent.width / 4 - width / 2
                font.pixelSize: _style.fontSizeEntries
            }
        }
    }

    Text {
        id: duration
        text: entry.durationString
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.margins: d.margin
        font.pixelSize: _style.fontSizeEntries
    }

    MouseArea {
        anchors.fill: parent
        onClicked: _app.playlist.play(root.entry)
    }
}
