import Qt 4.6

 Item {
     id: toggleswitch
     width: background.width + label.width + 40; height: background.height

     property string on: "false"
     property string text: "Toggle switch"

     function toggle() {
         if (toggleswitch.state == "on")
             toggleswitch.state = "off";
         else toggleswitch.state = "on";
     }

     function startup () {
	 if (toggleswitch.on == "1" ) 
       		toggleswitch.on = "true";
	 else 
		toggleswitch.on = "false";
         if (toggleswitch.on == "true")
             toggleswitch.state = "on";
         else
             toggleswitch.state = "off";
     }

     Component.onCompleted: startup();

     Text {
        id: label; text: toggleswitch.text; color: "White"; font.pointSize: 18;
        anchors.left: background.right; anchors.leftMargin: 32;
        anchors.verticalCenter: background.verticalCenter
     }


     Image {
         id: background; source: "background.svg"
         MouseRegion { anchors.fill: parent; onClicked: toggle() }
     }

     Image {
         id: knob; source: "knob.svg"; x: 1; y: 2

         MouseRegion {
             anchors.fill: parent
             drag.target: knob; drag.axis: "XAxis"; drag.minimumX: 1; drag.maximumX: 78
         }
     }

     states: [
         State {
             name: "on"
             PropertyChanges { target: knob; x: 78 }
         },
         State {
             name: "off"
             PropertyChanges { target: knob; x: 1 }
         }
     ]

     transitions: Transition {
         NumberAnimation { matchProperties: "x"; easing: "easeInOutQuad"; duration: 200 }
     }
 }
