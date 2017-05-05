import com.navit.graphics_qt5 1.0
import QtQuick 2.2
import QtQuick.Window 2.0

Window {
   width: 200; height: 200

   Loader {
       id: navit_loader
       focus: true
       source: "graphics_qt5.qml"
       anchors.fill: parent
       objectName: "navit_loader"
   }

   Item {
       id: root_item
       anchors.fill: parent
   }
}
