import QtQuick 2.11
import QtQuick.Controls 2.4

import Moosick 1.0 as Moosick

Rectangle {
    id: root

    color: "black"

    border.color: "white"
    border.width: 3

    SwipeView {
        anchors.fill: parent
        anchors.margins: 10

        SearchPane {
        }
    }
}
