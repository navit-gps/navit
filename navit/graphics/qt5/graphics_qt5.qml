import com.navit.graphics_qt5 1.0
import QtQuick 2.3
import QtQuick.Window 2.2

Window {
   width: 200; height: 200
   QNavitQuick {
       id: navit1
       anchors.fill: parent
       Component.onCompleted: {
          navit1.setGraphicContext(graphics_qt5_context)
       }
   }
}
