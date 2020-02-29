import QtQuick 2.11

import Moosick 1.0 as Moosick

Rectangle {
    id: root
    color: "black"

    ListView {
        anchors.fill: parent

        model: _app.database.searchResults
        spacing: 3
        flickableDirection: Flickable.AutoFlickIfNeeded

        delegate: LibraryArtist {
            id: libArtist
            artist: model.artist
            width: parent.width
        }
    }
}
