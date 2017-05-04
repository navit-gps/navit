import com.navit.graphics_qt5 1.0
import QtQuick 2.2
import QtQuick.Window 2.0

Rectangle {
   width: 200
   height: 100
   color: "red"

   Text {
       anchors.centerIn: parent
       text: "Hello, World!"
   }
   QNavitQuick {
        id: navit1
	width: 300
	height: 300
        focus: true
        Component.onCompleted: {
            navit1.setGraphicContext(graphics_qt5_context)
        }
    }
}
