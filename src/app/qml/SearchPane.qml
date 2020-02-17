import QtQuick 2.11

import Moosick 1.0 as Moosick

Flickable {
    id: root

    interactive: column.height > root.height
    contentWidth: column.width
    contentHeight: column.height

    Column {
        id: column
        width: root.width
        height: childrenRect.height

        Item {
            width: parent.width
            height: childrenRect.height + 10

            SimpleButton {
                id: retryButton
                anchors.left: parent.left
                anchors.margins: 20
                fontSize: 18
                label: "Retry"
                background: _app.search.hasErrors ? "black" : "darkgray"
                foreground: _app.search.hasErrors ? "white" : "lightgray"
                enabled: _app.search.hasErrors
                onClicked: _app.search.retry()
            }

            LineEdit {
                id: textInput
                anchors {
                    left: retryButton.right
                    right: searchButton.left
                    verticalCenter: retryButton.verticalCenter
                    margins: 20
                }
                height: retryButton.height - 10
                background: "black"
                foreground: "white"
                pixelSize: 16
            }

            SimpleButton {
                id: searchButton
                anchors.right: parent.right
                anchors.margins: 20
                fontSize: 18
                label: "Search"
                onClicked: _app.search.search(textInput.text)
            }
        }

        Rectangle {
            width: column.width
            height: 2
            color: "white"
        }

        Repeater {
            model: _app.search.model

            delegate: SearchResultDelegate {
                width: root.width
                result: model.result
            }
        }
    }
}
