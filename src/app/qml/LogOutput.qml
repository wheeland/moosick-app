import QtQuick 2.11

Rectangle {
    id: root
    color: "white"

    opacity: {
        if (visible && enabled) return 0.6;
        if (visible) return 0.3;
        return 0.0;
    }

    Behavior on opacity {
        NumberAnimation { duration: 150 }
    }

    ListView {
        id: logOutputColumn
        clip: true
        anchors.fill: parent
        model: root.visible ? _logger : null
        spacing: 2

        boundsBehavior: Flickable.OvershootBounds

        delegate: Rectangle {
            width: root.width
            height: text.height
            color: "white"
            Text {
                id: text
                width: root.width
                text: model.text
                wrapMode: Text.Wrap
                color: "black"
            }
        }
    }
}
