import QtQuick 2.11

import Moosick 1.0 as Moosick

Rectangle {
    id: root

    color: "black"

    readonly property Moosick.SelectTagsModel tagsModel : _app.database.tagsModel

    Item {
        id: header
        width: parent.width
        height: childrenRect.height

        SimpleButton {
            id: addTagButton
            anchors.horizontalCenter: parent.horizontalCenter
            label: "Add New Tag"
            onClicked: _app.database.addNewTag()
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
        id: tagsListView
        width: parent.width
        anchors.top: separator.bottom
        anchors.topMargin: 10
        anchors.bottom: parent.bottom
        clip: true
        spacing: 5
        flickableDirection: Flickable.AutoFlickIfNeeded

        model: root.tagsModel.tags

        delegate: Item {
            id: tagItem
            width: parent.width
            height: tagBubble.height + 4
            // please don't spam warnings T.T
            readonly property bool hasChildren: !((!model.tag) || (!model.tag.childTags) || (model.tag.childTags.size === 0))

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
                border.width: 2
            }

            Text {
                id: tagText
                anchors.verticalCenter: parent.verticalCenter
                x: 20 + 40 * model.offset
                text: model.tag.name
                color: "white"
                font.pixelSize: _style.fontSizeLabels
            }

            MultiChoice {
                options: tagItem.hasChildren ? ["Edit"] : ["Edit", "Delete"]
                anchors.fill: parent

                onSelected: {
                    if (index === 0) _app.database.editItem(model.tag);
                    if (index === 1) _app.database.removeItem(model.tag);
                }
            }
        }
    }
}
