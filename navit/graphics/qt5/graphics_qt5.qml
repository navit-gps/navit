import org.navitproject.graphics_qt5 1.0
import QtQuick 2.2
import QtQuick.Window 2.0

QNavitQuick {
    id: navit1
    anchors.fill: parent
    focus: true
    Component.onCompleted: {
        navit1.setGraphicContext(graphics_qt5_context)
    }
}
