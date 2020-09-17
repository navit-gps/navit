import QtQuick 2.11
import QtQuick.Window 2.10
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0

Window {

    id: window
    visible: true
    width: 800
    height: 450
    title: qsTr("Hello World")
    MainLayout{
        anchors.fill: parent
    }
    Settings {
        id:windowSettings
        property alias x: window.x
        property alias y: window.y
        property alias width: window.width
        property alias height: window.height
//        property alias modality : window.modality
    }
}
