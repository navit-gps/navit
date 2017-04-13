import com.navit.graphics_qt5 1.0
import QtQuick 2.2
import QtQuick.Window 2.0

Window {
   width: 800; height: 800

   Rectangle {
       width: 200
       height: 100
       color: "red"

       Text {
           anchors.centerIn: parent
           text: "Hello, World!"
       }
   }
}
