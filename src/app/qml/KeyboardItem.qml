import QtQuick 2.11
import QtQuick.VirtualKeyboard 2.4

Item {
    id: root
    height: inputPanel.height

    InputPanel {
        id: inputPanel

        anchors {
            left: parent.left
            leftMargin: -20
            right: parent.right
            rightMargin: -20
            bottom: parent.bottom
        }
    }

    Binding {
        target: inputPanel.keyboard.style
        property: 'keyboardDesignHeight'
        value: 1200
    }
}
