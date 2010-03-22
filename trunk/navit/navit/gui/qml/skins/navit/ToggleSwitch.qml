import Qt 4.6

 Item {
     id: toggleswitch
     width: background.width + label.width + gui.width/24; height: background.height

     property string on: "false"
     property string text: "Toggle switch"
     signal changed

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
        id: label; text: toggleswitch.text; color: "White"; font.pointSize: gui.height/32;
        anchors.left: background.right; anchors.leftMargin: gui.width/24;
        anchors.verticalCenter: background.verticalCenter
     }


     Image {
         id: background; source: "background.svg"
         MouseRegion { anchors.fill: parent; onClicked: toggle() }
	 height: gui.height/7.5; width: height*2.4;
     }

     Image {
         id: knob; source: "knob.svg"; x: 1; y: 2
	 height: gui.height/8; width: gui.height/8;

         MouseRegion {
             anchors.fill: parent
             drag.target: knob; drag.axis: "XAxis"; drag.minimumX: 1; drag.maximumX: background.width-knob.width
	     hoverEnabled: false; onReleased: toggle()
         }
     }

     states: [
         State {
             name: "on"
             PropertyChanges { target: knob; x: background.width-knob.width }
	     PropertyChanges { target: toggleswitch; on: "true" }
	     StateChangeScript { script: toggleswitch.changed(); }
         },
         State {
             name: "off"
             PropertyChanges { target: knob; x: 1 }
	     PropertyChanges { target: toggleswitch; on: "false" }
	     StateChangeScript { script: toggleswitch.changed(); }
         }
     ]

     transitions: Transition {
         NumberAnimation { matchProperties: "x"; easing: "easeInOutQuad"; duration: 200 }
     }
 }
