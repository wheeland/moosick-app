import QtQuick 2.11

import Moosick 1.0 as Moosick

Rectangle {
    id: root
    color: "black"

    property bool showingTagFilters: false

    readonly property Moosick.SelectTagsModel filterTagsModel : _app.database.filterTagsModel

    Item {
        id: header
        width: parent.width
        height: root.showingTagFilters ? (tagsListView.y + tagsListView.height) : clearButton.height
        Behavior on height { NumberAnimation { duration: 100 } }
        clip: true

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

        TagSelection {
            id: tagsListView
            visible: root.showingTagFilters
            anchors {
                left: parent.left
                right: parent.right
                top: clearButton.bottom
                topMargin: 10
            }
            height: 200

            tagsModel: root.filterTagsModel

            onClicked: {
                root.filterTagsModel.setSelected(tag, !selected);
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
