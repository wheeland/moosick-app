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
            height: childrenRect.height

            SimpleButton {
                id: retryButton
                anchors.left: parent.left
                anchors.margins: 20
                label: "Retry"
                background: _query.hasErrors ? "black" : "darkgray"
                foreground: _query.hasErrors ? "white" : "lightgray"
                enabled: _query.hasErrors
                onClicked: _query.retry()
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
                pixelSize: 14
            }

            SimpleButton {
                id: searchButton
                anchors.right: parent.right
                anchors.margins: 20
                label: "Search"
                onClicked: _query.search(textInput.text)
            }

        }

        Repeater {
            model: _query.model

            delegate: SearchResultDelegate {
                width: root.width
                result: model.result
            }
        }
    }
}