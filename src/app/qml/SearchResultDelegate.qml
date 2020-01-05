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
    }

    Image {
        anchors.verticalCenter: titleText.verticalCenter
        x: 100
        height: 30
        source: root.result.iconData
        fillMode: Image.PreserveAspectFit
    }

    Text {
        anchors { right: parent.right; top: parent.top; margins: 10 }
        color: root.textColor
        text: {
            switch (root.result.status) {
            case SearchResult.InfoOnly: return "InfoOnly";
            case SearchResult.Querying: return "Querying";
            case SearchResult.Error: return "Error";
            case SearchResult.Done: return "Done";
            default: return "null";
            }
        }
        font.italic: true
    }

    Text {
        id: titleText
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; margins: 10 }
        color: root.textColor
        text: root.result.title
        font.bold: true
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.result.queryInfo();
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

    SearchResultPopup {
        id: popup
        visible: false
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        preventStealing: true

        onPressAndHold: popup.show(mouse.x, mouse.y)
        onReleased: {
            if (popup.selected1) {

            }

            popup.hide()
        }
        onPositionChanged: popup.move(mouse.x, mouse.y)
    }
}
