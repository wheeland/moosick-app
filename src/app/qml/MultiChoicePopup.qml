import QtQuick 2.11

import Moosick 1.0

Item {
    id: root

    visible: _multiChoiceController.visible

    readonly property real radius: 60
    readonly property real choiceRadius: 30

    Item {
        id: optionsItem

        Repeater {
            model: _multiChoiceController.options

            delegate: Rectangle {
                id: optionDelegate

                readonly property real angle: index * 2.0 * 3.141592 / _multiChoiceController.options.length

                x: _multiChoiceController.centerX + root.radius * Math.sin(angle) - width/2
                y: _multiChoiceController.centerY + root.radius * Math.cos(angle) - height/2

                width: optionText.width + 40
                height: optionText.height + 20
                radius: height / 4

                color: selected ? "#444444" : "#222222"
                border.width: 2
                border.color: "white"

                Text {
                    id: optionText
                    anchors.centerIn: parent
                    text: _multiChoiceController.options[index]
                    color: "white"
                }

                readonly property bool selected:
                    (_multiChoiceController.mouseX >= x) &&
                    (_multiChoiceController.mouseX <= x + width) &&
                    (_multiChoiceController.mouseY >= y) &&
                    (_multiChoiceController.mouseY <= y + height)

                onSelectedChanged: _multiChoiceController.setSelected(index, selected)
            }
        }
    }
}
