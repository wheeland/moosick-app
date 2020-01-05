import QtQuick 2.11

Image {
    id: root

    property real center
    property real size
    property bool enabled

    signal clicked()

    x: center - 0.5 * width
    height: size
    anchors.verticalCenter: parent.verticalCenter

    fillMode: Image.PreserveAspectFit
    mipmap: true
    opacity: enabled ? 1.0 : 0.3

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (root.enabled)
                root.clicked()
        }
    }
}
