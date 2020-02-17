import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQml 2.11 as QQML
import QtMultimedia 5.11 as QQM
import QtQuick.Layouts 1.11 as QQL

Item {
    id: root

    readonly property real scale: width / _defaultWindowWidth

    Binding {
        target: _multiChoiceController
        property: "uiScale"
        value: root.scale
    }

    MainWindow {
        width: parent.width / root.scale
        height: parent.height / root.scale
        anchors.centerIn: parent
        scale: root.scale
    }
}
