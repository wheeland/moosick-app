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
        text: root.artist ? root.artist.name : ""
        font.pixelSize: textSize
        font.bold: true
    }

    LibraryTagSpace {
        id: tagSpace
        model: root.artist ? root.artist.tags : null
        anchors {
            left: artistNameText.right
            leftMargin: 50
            top: parent.top
            right: parent.right
            margins: 5
        }
        height: Math.max(childrenRect.height, artistNameText.height)
    }

    MultiChoice {
        options: ["Play", "Edit", "Remove"]
        anchors.fill: parent
        anchors.bottomMargin: albumsColumn.height

        onClicked: {
            _app.database.fillArtistInfo(root.artist);
            root.expanded = !expanded;
        }

        onSelected: {
            if (index === 0) _app.addLibraryItemToPlaylist(root.artist, true);
            if (index === 1) _app.database.editItem(root.artist);
            if (index === 2) _app.database.removeItem(root.artist);
        }
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
            model: (root.expanded && root.artist) ? root.artist.albums : null

            delegate: LibraryAlbum {
                album: model.album
                width: parent.width
            }
        }
    }
}
