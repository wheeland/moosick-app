import QtQuick 2.11

import Moosick 1.0 as Moosick

Rectangle {
    id: root

    property string label
    property Moosick.StringModel stringsModel
    property Moosick.SelectTagsModel tagsModel

    signal okClicked()
    signal cancelClicked()

    color: "black"
    property real fontSize: 20

    border.color: "white"
    border.width: 3

    property bool showTags: false

    Connections {
        target: root.stringsModel
        onPopup: {
            textInput.text = initialString
            root.showTags = false;
        }
    }

    SimpleButton {
        id: cancelButton
        label: "Cancel"
        width: parent.width * 0.22
        x: 0.2 * parent.width - 0.5 * width
        y: parent.width / 16
        onClicked: root.cancelClicked()
    }

    SimpleButton {
        id: okButton
        label: "OK"
        width: parent.width * 0.22
        x: 0.8 * parent.width - 0.5 * width
        y: parent.width / 16
        onClicked: {
            if (textInput.hasInputFocus)
                textInput.hasInputFocus = false;
            else
                root.okClicked();
        }
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

    Item {
        id: contentItem

        anchors {
            left: parent.left
            right: parent.right
            top: (root.label.length > 0) ? labelText.bottom : okButton.bottom
            bottom: parent.bottom
            margins: 20
        }

        SimpleButton {
            id: clearButton
            anchors {
                right: parent.right
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
                rightMargin: 20
                right: clearButton.left
                top: parent.top
            }
            background: hasInputFocus ? "#333333" : "black"
            foreground: "white"
            pixelSize: root.fontSize

            onTextChanged: root.stringsModel.entered(text)
        }

        Rectangle {
            id: additionalContentItem
            anchors {
                left: parent.left
                right: parent.right
                top: textInput.bottom
                topMargin: 20
                bottom: parent.bottom
            }
            color: "black"
            border.color: "white"
            border.width: 1

            ListView {
                id: entriesListView
                visible: textInput.hasInputFocus && _app.database.editItemStringsChoiceActive
                anchors.fill: parent
                anchors.margins: 8
                clip: true

                model: root.stringsModel

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
                            textInput.text = model.text;
                        }
                    }
                }
            }

            ListView {
                id: tagsListView
                visible: !entriesListView.visible
                anchors.fill: parent
                anchors.margins: 8
                clip: true

                model: root.tagsModel.tags

                delegate: Item {
                    width: parent.width
                    height: tagBubble.height + 4

                    Rectangle {
                        id: tagBubble
                        anchors {
                            fill: tagText
                            margins: -3
                            leftMargin: -10
                            rightMargin: -10
                        }
                        radius: height / 2
                        color: "#333333"
                        border.color: "#cccccc"
                        border.width: model.selected ? 2 : 0
                    }

                    Text {
                        id: tagText
                        anchors.verticalCenter: parent.verticalCenter
                        x: 20 + 40 * model.offset
                        text: model.tag.name
                        color: "white"
                        font.pixelSize: root.fontSize
                    }

                    MouseArea {
                        anchors.fill: tagBubble
                        onClicked: {
                            root.tagsModel.setSelected(model.tag, !model.selected);
                        }
                    }
                }
            }
        }
    }
}
