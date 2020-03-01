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

    TagSelection {
        id: tagsListView
        width: parent.width
        anchors.top: separator.bottom
        anchors.topMargin: 10
        anchors.bottom: parent.bottom

        tagsModel: root.tagsModel
        alwaysOutline: true

        function multiChoiceOptions(tag) {
            var hasChildren = tag && tag.childTags && (tag.childTags.size > 0);
            return hasChildren ? ["Edit"] : ["Edit", "Delete"];
        }

        onSelected: {
            if (index === 0) _app.database.editItem(tag);
            if (index === 1) _app.database.removeItem(tag);
        }
    }
}
