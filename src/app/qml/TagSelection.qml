import QtQuick 2.11

import Moosick 1.0 as Moosick

ListView {
    id: root
    clip: true
    spacing: 5
    flickableDirection: Flickable.AutoFlickIfNeeded

    property real fontSize: _style.fontSizeEntries
    property bool alwaysOutline: false
    property Moosick.SelectTagsModel tagsModel

    signal clicked(var tag, var selected)
    signal selected(var tag, var selected, var index)

    model: tagsModel ? tagsModel.tags : null

    function multiChoiceOptions(tag) {
        return [];
    }

    delegate: Item {
        id: tagItem
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
            border.width: (root.alwaysOutline || model.selected) ? 2 : 0
        }

        Text {
            id: tagText
            anchors.verticalCenter: parent.verticalCenter
            x: 20 + 40 * model.offset
            text: model.tag.name
            color: "white"
            font.pixelSize: root.fontSize
        }

        MultiChoice {
            anchors.fill: tagBubble
            options: root.multiChoiceOptions(model.tag)
            onSelected: root.selected(model.tag, model.selected, index)
            onClicked: root.clicked(model.tag, model.selected)
        }
    }
}
