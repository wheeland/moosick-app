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

        onClicked: {
            root.clicked();
        }

        onPressAndHold: {
            preventStealing = true;
            _multiChoiceController.show(root, root.options, mouse.x, mouse.y);
        }

        onReleased: {
            preventStealing = false;
            if (_multiChoiceController.selected >= 0)
                root.selected(_multiChoiceController.selected);
            _multiChoiceController.hide();
        }

        onPositionChanged: {
            _multiChoiceController.move(mouse.x, mouse.y);
        }
    }
}
