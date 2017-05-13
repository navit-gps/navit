import com.navit.graphics_qt5 1.0
import QtQuick 2.2
// import QtQuick.Controls 1.2

Rectangle {
    width: 800
    height: 480
	Connections {
		target: backend
		onSetTextField: textField1.text = text
		onDisplayMenu: {
			mainMenu.source = "skins/menu.qml"
			mainMenu.state = 'visible'
			console.log("showing menu")
		}
	}
	color: "black"
	id: container

	QNavitQuick {
		id: navit1
		width: parent.width
		height: parent.height
		focus: true
		opacity: 0;
		Component.onCompleted: {
			navit1.setGraphicContext(graphics_qt5_context);
			navit1.opacity = 1;
		}
		Behavior on opacity {
			NumberAnimation {
				id: opacityAnimation;duration: 1000;alwaysRunToEnd: true
			}
		}
	}

	Loader {
		id: mainMenu
		width: parent.width
		height: parent.height
		x: parent.width
		opacity: 0

		states: [
			State {
				name: "visible"
				PropertyChanges {
					target: mainMenu
					x: 0
					opacity: 1
				}
			}
		]
		transitions: [
			Transition {
				NumberAnimation {
					properties: "x,y,opacity";duration: 200
				}
			}
		]
	}
}
