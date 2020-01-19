import QtQuick 2.11

import Moosick 1.0 as Moosick

Item {
    id: root

    Flickable {
        id: flickable

        anchors.fill: parent
        interactive: column.height > root.height
        contentWidth: column.width
        contentHeight: column.height

        Column {
            id: column
            width: root.width
            height: childrenRect.height

            Repeater {
                model: _app.database.rootTags

                delegate: TagBubble {
                    tag: model.tag
                }
            }
        }
    }
}
