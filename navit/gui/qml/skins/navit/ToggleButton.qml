import Qt 4.7

 Item {
     id: togglebutton
     width: imgItem.width + txtItem.width + gui.width/24; height: imgItem.height

     property string stOn: "false"
     property string text: "Toggle button"
    property string icon: "Icon.png"

     signal changed

     function toggle() {
         if (togglebutton.state == "on")
             togglebutton.state = "off";
         else
	     togglebutton.state = "on";
     }

     function startup () {
	 if (togglebutton.stOn == "1" )
       		togglebutton.stOn = "true";
	 else if (togglebutton.stOn == "0" )
		togglebutton.stOn = "false";
         if (togglebutton.stOn == "true")
             togglebutton.state = "on";
         else
             togglebutton.state = "off";
     }

     Component.onCompleted: startup();

    MouseArea { id: mr; anchors.fill: parent; onReleased: toggle() }

   Image {
	id: imgItem; source: gui.iconPath+togglebutton.icon; anchors.top: togglebutton.top; anchors.horizontalCenter: togglebutton.horizontalCenter;
	width: gui.height/32; height: gui.height/32
   }

    Text {
        id: txtItem; text: togglebutton.text; anchors.top: imgItem.bottom; anchors.horizontalCenter: togglebutton.horizontalCenter;
	color: "White"; font.pointSize: gui.height/64; horizontalAlignment: Qt.AlignHCenter
    }

     states: [
         State {
             name: "on"
             PropertyChanges { target: imgItem; opacity: 1 }
	     PropertyChanges { target: txtItem; opacity: 1 }
	     PropertyChanges { target: togglebutton; stOn: "true" }
	     StateChangeScript { script: togglebutton.changed(); }
         },
         State {
             name: "off"
             PropertyChanges { target: imgItem; opacity: 0.3 }
	     PropertyChanges { target: txtItem; opacity: 0.3 }
	     PropertyChanges { target: togglebutton; stOn: "false" }
	     StateChangeScript { script: togglebutton.changed(); }
         }
     ]

    transitions: Transition {
        NumberAnimation { properties: "scale"; easing.type: "OutExpo"; duration: 200 }
        NumberAnimation { properties: "opacity"; easing.type: "InQuad"; duration: 200 }
    }
 }
