import QtQuick 2.11
import QtQuick.Layouts 1.11

import Moosick 1.0

Rectangle {
    id: root

    property DbArtist artist
    property bool expanded: false
    property color textColor: "#cccccc"
    property real textSize: 18
    property real slideDuration: 100

    height: tagSpace.height + albumsColumn.height + 10
    color: "#222222"
    clip: true

    Text {
        id: artistNameText
        x: 10
        anchors.verticalCenter: tagSpace.verticalCenter
        color: root.textColor
        text: root.artist.name
        font.pixelSize: textSize
        font.bold: true
    }

    LibraryTagSpace {
        id: tagSpace
        model: root.artist.tags
        anchors {
            left: artistNameText.right
            leftMargin: 50
            top: parent.top
            right: parent.right
            margins: 5
        }
        height: Math.max(childrenRect.height, artistNameText.height)
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent

        onClicked: {
            _app.database.fillArtistInfo(root.artist);
            root.expanded = !expanded;
        }
        onPressAndHold: {
            preventStealing = true;
//            popup.show(mouse.x, mouse.y);
        }
        onReleased: {
            preventStealing = false;

//            if (popup.selected1)
//                _app.addToPlaylist(root.result, false);
//            if (popup.selected2)
//                _app.addToPlaylist(root.result, true);

//            popup.hide()
        }
//        onPositionChanged: popup.move(mouse.x, mouse.y)
    }

    Column {
        id: albumsColumn

        height: (childrenRect.height > 0) ? (childrenRect.height + 10) : 0

        anchors {
            left: parent.left
            leftMargin: 5
            top: tagSpace.bottom
            topMargin: 10
            right: parent.right
            rightMargin: 5
        }

        spacing: 5

        Repeater {
            model: root.expanded ? root.artist.albums : null

            delegate: LibraryAlbum {
                album: model.album
                width: parent.width
            }
        }
    }
}