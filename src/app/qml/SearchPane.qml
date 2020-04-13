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

            LineEdit {
                id: textInput
                anchors {
                    left: parent.left
                    right: searchButton.left
                    margins: 20
                }
                background: "black"
                foreground: "white"
                pixelSize: _style.fontSizeButtons * 0.8
            }

            SimpleButton {
                id: searchButton
                anchors.right: parent.right
                anchors.margins: 20
                label: "Search"
                onClicked: {
                    textInput.hasInputFocus = false;
                    _app.search.search(textInput.displayText);
                }
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
