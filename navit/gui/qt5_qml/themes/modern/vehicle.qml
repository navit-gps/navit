import QtQuick 2.0

Item {
    Text {
        x: 8
        y: 8
        color: "#ffffff"
        text: backend.currentVehicle.name
        font.pixelSize: 32
    }
}
