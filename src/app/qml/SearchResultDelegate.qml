import QtQuick 2.11

import Moosick 1.0 as Moosick

Item {
    id: root

    property Moosick.Result result

    height: 50 + childColumn.childrenRect.height

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        color: "#333333"
        border.width: 2
        border.color: "white"
    }

    property color textColor: "#cccccc"
    property int childResults: {
        switch (root.result.type) {
        case Moosick.Result.BandcampArtist: return root.result.albumCount;
        case Moosick.Result.BandcampAlbum: return root.result.trackCount;
        default: return 0;
        }
    }

    function childResult(index) {
        switch (root.result.type) {
        case Moosick.Result.BandcampArtist: return root.result.getAlbum(index);
        case Moosick.Result.BandcampAlbum: return root.result.getTrack(index);
        default: return null;
        }
    }

    Text {
        anchors { left: parent.left; top: parent.top; margins: 10 }
        color: root.textColor
        text: {
            switch (root.result.type) {
            case Moosick.Result.BandcampArtist: return "artist";
            case Moosick.Result.BandcampAlbum: return "album";
            case Moosick.Result.BandcampTrack: return "track";
            case Moosick.Result.YoutubeVideo: return "video";
            case Moosick.Result.YoutubePlaylist: return "playlist";
            default: return "null";
            }
        }
        font.italic: true
    }

    Image {
        anchors.verticalCenter: parent.verticalCenter
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
            case Moosick.Result.InfoOnly: return "InfoOnly";
            case Moosick.Result.Querying: return "Querying";
            case Moosick.Result.Error: return "Error";
            case Moosick.Result.Done: return "Done";
            default: return "null";
            }
        }
        font.italic: true
    }

    Text {
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
        visible: (root.childResults > 0)
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
                model: root.childResults

                Loader {
                    active: true
                    source: "SearchResultDelegate.qml"
                    onItemChanged: {
                        item.width = childColumn.width;
                        item.result = root.childResult(index)
                    }
                }
            }
        }
    }
}
