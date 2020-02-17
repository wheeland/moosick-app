import QtQuick 2.11

import Moosick 1.0

Item {
    id: root

    property var options: []

    signal clicked()
    signal selected(var index)

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        pressAndHoldInterval: 250

        property bool longPressed: false

        onClicked: {
            root.clicked();
        }

        onPressAndHold: {
            if (root.options.length === 0)
                return;

            longPressed = true;
            preventStealing = true;
            _multiChoiceController.show(root, root.options, mouse.x, mouse.y);
            _multiChoiceController.move(mouse.x, mouse.y);
        }

        onReleased: {
            preventStealing = false;

            if (root.options.length > 0 && mouseArea.longPressed) {
                if (_multiChoiceController.selected >= 0)
                    root.selected(_multiChoiceController.selected);
            }
            _multiChoiceController.hide();
        }

        onPositionChanged: {
            if (root.options.length === 0)
                return;

            _multiChoiceController.move(mouse.x, mouse.y);
        }
    }
}
