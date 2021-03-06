import QtQuick 2.11

import Moosick 1.0

Item {
    id: root

    property SearchResult result

    height: 50 + childColumn.childrenRect.height

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        color: "#333333"
        border.width: 2
        border.color: "white"
    }

    property color textColor: "#cccccc"

    Text {
        anchors { left: parent.left; top: parent.top; margins: 10 }
        color: root.textColor
        text: {
            switch (root.result.type) {
            case SearchResult.BandcampArtist: return "artist";
            case SearchResult.BandcampAlbum: return "album";
            case SearchResult.BandcampTrack: return "track";
            case SearchResult.YoutubeVideo: return "video";
            case SearchResult.YoutubePlaylist: return "playlist";
            default: return "null";
            }
        }
        font.italic: true
        font.pixelSize: _style.fontSizeEntries
    }

    Image {
        anchors.top: titleText.top
        anchors.bottom: titleText.bottom
        x: 100
        source: root.result.iconData
        fillMode: Image.PreserveAspectFit
    }

    Text {
        anchors { right: parent.right; top: parent.top; margins: 10 }
        color: root.textColor
        text: {
            switch (root.result.downloadStatus) {
            case SearchResult.DownloadNotStarted: return "";
            case SearchResult.DownloadStarted: return "Downloading..";
            case SearchResult.DownloadDone: return "Downloaded!";
            default: return "";
            }
        }
        font.italic: true
        font.pixelSize: _style.fontSizeEntries
    }

    Text {
        id: titleText
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; margins: 10 }
        color: root.textColor
        text: {
            switch (root.result.type) {
            case SearchResult.BandcampAlbum: return root.result.artist + " - " + root.result.title
            case SearchResult.BandcampTrack: return root.result.album.artist + " - " + root.result.album.title + " - " + root.result.title
            default: root.result.title
            }
        }
        font.bold: true
        font.pixelSize: _style.fontSizeEntries
    }

    MultiChoice {
        options: {
            switch (root.result.type) {
            case SearchResult.BandcampArtist: return [];
            case SearchResult.BandcampAlbum: return ["Prepend", "Append", "Download"];
            case SearchResult.BandcampTrack: return ["Prepend", "Append"];
            case SearchResult.YoutubeVideo: return ["Prepend", "Append", "Download"];
            case SearchResult.YoutubePlaylist: return ["Prepend", "Append"];
            default: return [];
            }
        }
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: childRect.visible ? childRect.top : parent.bottom
        }

        onClicked: root.result.queryInfo()
        onSelected: {
            if (index === 0) _app.addSearchResultToPlaylist(root.result, false);
            else if (index === 1) _app.addSearchResultToPlaylist(root.result, true);
            else if (index === 2) _app.download(root.result);
        }
    }

    Rectangle {
        id: childRect
        visible: (repeater.count > 0)
        anchors.fill: parent
        anchors.margins: 5
        anchors.leftMargin: 20
        anchors.topMargin: 41
        color: "#333333"
        border.width: 1
        border.color: "white"

        Column {
            id: childColumn
            anchors.fill: parent
            anchors.margins: 2

            Repeater {
                id: repeater

                model: {
                    switch (root.result.type) {
                    case SearchResult.BandcampArtist: return root.result.albums;
                    case SearchResult.BandcampAlbum: return root.result.tracks;
                    default: return undefined;
                    }
                }

                Loader {
                    active: true
                    source: "SearchResultDelegate.qml"
                    onItemChanged: {
                        item.width = childColumn.width;
                        item.result = model.result;
                    }
                }
            }
        }
    }
}
