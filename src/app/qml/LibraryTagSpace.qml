import QtQuick 2.11
import QtQuick.Layouts 1.11

import Moosick 1.0

Flow {
    id: root

    property alias model: repeater.model

    height: childrenRect.height

    layoutDirection: Flow.RightToLeft
    spacing: 5

    Repeater {
        id: repeater
        model: root.model
        delegate: TagBubble {
            tag: model.tag
        }
    }
}
