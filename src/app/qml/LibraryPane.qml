import QtQuick 2.11

import Moosick 1.0 as Moosick

Rectangle {
    id: root
    color: "black"

    property bool showingTagFilters: false

    Item {
        id: header
        width: parent.width
        height: childrenRect.height

        SimpleButton {
            id: filterButton
            anchors.left: parent.left
            anchors.margins: 20
            label: "Filters"
            background: root.showingTagFilters ? "#555555" : "#333333"
            onClicked: root.showingTagFilters = !root.showingTagFilters
        }

        LineEdit {
            id: textInput
            anchors {
                left: filterButton.right
                right: clearButton.left
                verticalCenter: filterButton.verticalCenter
                margins: 20
            }
            height: filterButton.height - 10
            background: "black"
            foreground: "white"
            pixelSize: _style.fontSizeButtons * 0.8

            onTextChanged: _app.database.search(text)
        }

        SimpleButton {
            id: clearButton
            anchors.right: parent.right
            anchors.margins: 20
            label: "Clear"
            onClicked: {
                textInput.hasInputFocus = false;
                textInput.text = "";
            }
        }
    }

    Rectangle {
        id: separator
        width: parent.width
        y: header.y + header.height + 10
        height: 2
        color: "white"
    }

    ListView {
        width: parent.width
        anchors.top: separator.bottom
        anchors.topMargin: 10
        anchors.bottom: parent.bottom
        clip: true

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
