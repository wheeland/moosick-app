import QtQuick 2.11
import QtMultimedia 5.11

Rectangle {
    id: root
    color: "#333333"
    height: d.buttonsHeight + 3  * d.offsets + d.playbarHeight + d.songInfoHeight

    QtObject {
        id: d
        readonly property real offsets: 10
        readonly property real buttonsHeight: 80
        readonly property real playbarHeight: 50
        readonly property real songInfoHeight: 40
    }

    Item {
        id: buttonRow
        width: parent.width
        y: d.offsets
        height: d.buttonsHeight

        PlayerBarButton {
            center: parent.width * 0.2
            size: parent.height
            source: "../data/back.png"
            enabled: _app.audio.hasSong
            onClicked: _app.playlist.next()
        }

        PlayerBarButton {
            center: parent.width * 0.5
            size: parent.height
            visible: !_app.audio.playing
            source: "../data/play.png"
            enabled: _app.audio.hasSong
            onClicked: _app.audio.play()
        }

        PlayerBarButton {
            center: parent.width * 0.5
            size: parent.height
            visible: _app.audio.playing
            source: "../data/pause.png"
            enabled: _app.audio.hasSong
            onClicked: _app.audio.pause()
        }

        PlayerBarButton {
            center: parent.width * 0.8
            size: parent.height
            source: "../data/forward.png"
            enabled: _app.audio.hasSong
            onClicked: _app.playlist.previous()
        }
    }

    Item {
        id: seekerBandItem
        anchors.top: buttonRow.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: d.offsets
        height: d.playbarHeight

        property bool peeking : false
        property real peekPos : 0.0
        readonly property real displayPos : peeking ? peekPos : _app.audio.position

        function doPeek(pos) {
            peeking = true;
            peekPos = Math.max(0, Math.min(pos, 1));
        }

        function donePeek() {
            peeking = false;
            _app.audio.seek(peekPos);
        }

        Text {
            id: leftText
            text: _app.audio.timeToDisplayString(seekerBandItem.displayPos)
            color: "white"
            font.pixelSize: 20
            width: 70
            horizontalAlignment: Text.AlignRight
        }

        Item {
            id: seekerBand
            anchors.left: leftText.right
            anchors.right: rightText.left
            anchors.verticalCenter: leftText.verticalCenter
            anchors.margins: 20
            height: parent.height

            Rectangle {
                width: parent.width
                anchors.verticalCenter: parent.verticalCenter
                height: 3
                color: "white"
            }

            Rectangle {
                id: knob
                x: parent.width * seekerBandItem.displayPos - width / 2
                anchors.verticalCenter: seekerBand.verticalCenter
                width: 0.6 * parent.height
                height: width
                radius: width / 2
                color: "white"
            }

            MouseArea {
                anchors.fill: parent
                onPressed: seekerBandItem.doPeek(mouse.x / width)
                onPositionChanged: seekerBandItem.doPeek(mouse.x / width)
                onReleased: seekerBandItem.donePeek()
            }
        }

        Text {
            id: rightText
            text: _app.audio.durationString
            color: "white"
            font.pixelSize: 20
            width: 70
            horizontalAlignment: Text.AlignLeft
            anchors.right: parent.right
        }
    }

    Item {
        id: songInfo
        anchors.top: seekerBandItem.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: d.offsets
        height: d.songInfoHeight

        Text {
            anchors.centerIn: parent
            text: {
                if (!_app.playlist.currentSong)
                    return "";
                return _app.playlist.currentSong.artist + " - " + _app.playlist.currentSong.album + " - " + _app.playlist.currentSong.title
            }
        }
    }

//    Column {
//        anchors.top: playButton.bottom
//        Text { color: "white"; text: "Availability: " + availability2str(audio.availability) }
//        Text { color: "white"; text: "Status: " + status2str(audio.status) }
//        Text { color: "white"; text: "Buffer: " + audio.bufferProgress }
//        Text { color: "white"; text: "Error: " + audio.errorString }
//        Text { color: "white"; text: "hasAudio: " + audio.hasAudio }
//        Text { color: "white"; text: "Playback Rate: " + audio.playbackRate.toFixed(2) }
//        Text { color: "white"; text: "Playback State: " + playbackState2str(audio.playbackState) }
//        Text { color: "white"; text: "seekable: " + audio.seekable }
//    }
}
