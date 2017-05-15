import com.navit.graphics_qt5 1.0
import QtQuick 2.2
import QtQuick.Window 2.0

Window {
   width: 200; height: 200
    
   Screen.orientationUpdateMask: Qt.PortraitOrientation + Qt.LandscapeOrientation + Qt.InvertedPortraitOrientation + Qt.InvertedLandscapeOrientation

   Item {
       id: root
       anchors.fill: parent
       Loader {
           width: root.width
	   height: root.height
	   anchors.centerIn: parent
	   Screen.onOrientationChanged: {
	       rotation = Screen.angleBetween(Screen.orientation , Screen.primaryOrientation)
	       switch(Screen.angleBetween(Screen.orientation , Screen.primaryOrientation))
	       {
	           case 0:
		   case 180:
		       width = root.width
		       height = root.height
		   break
		   case 90:
		   case 270:
		   width = root.height
		   height = root.width
		   break
	       }
           }
           id: navit_loader
           focus: true
           source: "graphics_qt5.qml"
           objectName: "navit_loader"
       }
   }
}
