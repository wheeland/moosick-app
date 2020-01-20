import QtQuick 2.11

import Moosick 1.0 as Moosick

Rectangle {
    id: root

    property string label
    property Moosick.StringModel model

    color: "black"
    property real fontSize: 20

    border.color: "white"
    border.width: 3

    SimpleButton {
        id: cancelButton
        label: "Cancel"
        width: parent.width / 4
        x: parent.width / 8
        y: parent.width / 16
    }

    SimpleButton {
        id: okButton
        label: "OK"
        width: parent.width / 4
        x: 5 * parent.width / 8
        y: parent.width / 16
    }

    Text {
        id: labelText
        anchors {
            left: parent.right
            right: parent.right
            top: okButton.bottom
            margins: 20
        }
        text: root.label
        font.pixelSize: root.fontSize * 1.5
        wrapMode: Text.WordWrap
        color: "white"
        horizontalAlignment: Text.AlignHCenter
    }

    SimpleButton {
        id: clearButton
        anchors {
            right: parent.right
            rightMargin: 20
            top: textInput.top
            bottom: textInput.bottom
        }
        label: "X"
        width: height
        onClicked: textInput.text = ""
    }

    LineEdit {
        id: textInput
        anchors {
            left: parent.left
            right: clearButton.left
            top: (root.label.length > 0) ? labelText.bottom : okButton.bottom
            margins: 20
        }
        background: "black"
        foreground: "white"
        pixelSize: root.fontSize

        onTextChanged: root.model.entered(text)
    }

    Rectangle {
        id: entriesRect
        anchors {
            left: parent.left
            right: parent.right
            top: textInput.bottom
            bottom: parent.bottom
            margins: 20
        }

        color: "black"
        border.color: "white"
        border.width: 1

        ListView {
            id: entriesListView
            anchors.fill: parent
            anchors.margins: 8
            clip: true

            model: root.model

            delegate: Item {
                width: parent.width
                height: entryText.height + 8

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 2
                    radius: height / 2
                    color: "#333333"
                    border.color: "#cccccc"
                    border.width: model.selected ? 2 : 0
                }

                Text {
                    id: entryText
                    anchors.verticalCenter: parent.verticalCenter
                    x: height / 2
                    text: model.text
                    color: "white"
                    font.pixelSize: root.fontSize
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.model.select(index);
                        textInput.text = model.text;
                    }
                }
            }
        }
    }
}
