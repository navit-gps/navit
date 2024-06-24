import org.navitproject.navit.graphics_qt5 1.0
import QtQuick 2.4
import Lomiri.Components 1.3
import QtQuick.Window 2.0

Window {
   width: 200; height: 200

    ScreenSaver {
        id: screenSaver
        screenSaverEnabled: !Core.settings.disableScreensaver
    }

    Connections {
        target: Qt.application
        onActiveChanged: {
            if (!Qt.application.active) {
                screenSaver.screenSaverEnabled = false
            } else {
                screenSaver.screenSaverEnabled = !Core.settings.disableScreensaver
            }
        }
    }

    Item {
        id: root
        anchors.fill: parent
        Loader {
            width: root.width
	    height: root.height
	    anchors.centerIn: parent
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
