import com.navit.graphics_qt5 1.0
import QtQuick 2.3
import QtQuick.Window 2.2

Window {
   QNavitQuick {
       id: aPieChart
       anchors.centerIn: parent
       width: 100; height: 100
       name: "A simple pie chart"
       color: "red"
   }
}
