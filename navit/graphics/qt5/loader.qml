import com.navit.graphics_qt5 1.0
import QtQuick 2.2
import QtQuick.Window 2.0

Window {
   width: 200; height: 200

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
       Component.onCompleted: {
           // orientation update mask is defined since QML 5.4 So make this compatible to 5.2
	   // by just calling this if available
           if(Screen.hasOwnProperty('orientationUpdateMask')) {
               Screen.orientationUpdateMask = Qt.PortraitOrientation + Qt.LandscapeOrientation + Qt.InvertedPortraitOrientation + Qt.InvertedLandscapeOrientation;
	   }
       }
   }
}
