import QtQuick 2.11

import Moosick 1.0 as Moosick

Rectangle {
    id: root

    color: "black"

    border.color: "white"
    border.width: 3

    Flickable {
        id: flickable
        anchors.fill: parent
        anchors.margins: 10

        contentWidth: column.width
        contentHeight: column.height

        Column {
            id: column
            width: flickable.width
            height: childrenRect.height

            SimpleButton {
                label: "Retry"
                background: _query.hasErrors ? "black" : "darkgray"
                foreground: _query.hasErrors ? "white" : "lightgray"
                enabled: _query.hasErrors
                onClicked: _query.retry()
            }

            Repeater {
                model: _query
                delegate: Rectangle {
                    width: parent.width - 50
                    height: 80

                    color: "#333333"
                    border.width: 3
                    border.color: {
                        if (model.result.type === Moosick.Result.BandcampArtist)
                            return "#002288";
                        if (model.result.type === Moosick.Result.BandcampAlbum)
                            return "#0066ff";
                        return "black";
                    }

                    Text {
                        anchors.centerIn: parent
                        text: model.result.title + " - " + model.result.url
                    }

                }
            }
        }
    }
}
