import QtQuick 2.11

import Moosick 1.0

Item {
    id: root

    property real cx
    property real cy

    property bool selected1: false
    property bool selected2: false

    function show(xx, yy) {
        cx = xx;
        cy = yy;
        selected1 = false;
        selected2 = false;
        visible = true;
    }

    function hide() {
        visible = false;
    }

    function move(xx, yy) {
        var inside = (xx > x && xx < x + width && yy > y && yy < y + height);
        selected1 = inside && (xx < cx);
        selected2 = inside && (xx > cx);
    }

    x: cx - width / 2
    y: cy - height / 2

    width: 300
    height: 40

    Rectangle {
        width: parent.width / 2
        height: parent.height
        color: root.selected1 ? "#666666" : "#222222"
        border.width: 2
        border.color: "white"
    }

    Rectangle {
        x: width
        width: parent.width / 2
        height: parent.height
        color: root.selected2 ? "#666666" : "#222222"
        border.width: 2
        border.color: "white"
    }
}
