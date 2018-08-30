import Qt 4.7

 Item {
     id: toggleswitch
     width: background.width + label.width + gui.width/24; height: background.height

     property string stOn: "false"
     property string text: "Toggle switch"
     signal changed

     function toggle() {
         if (toggleswitch.state == "on")
             toggleswitch.state = "off";
         else toggleswitch.state = "on";
     }

     function startup () {
	 if (toggleswitch.stOn == "1" )
       		toggleswitch.stOn = "true";
	 else if (toggleswitch.stOn == "0" )
		toggleswitch.stOn = "false";
         if (toggleswitch.stOn == "true")
             toggleswitch.state = "on";
         else
             toggleswitch.state = "off";
     }

     Component.onCompleted: startup();

     Text {
        id: label; text: toggleswitch.text; color: "White"; font.pointSize: gui.height/20;
        anchors.left: background.right; anchors.leftMargin: gui.width/24;
        anchors.verticalCenter: background.verticalCenter
     }


     Image {
         id: background; source: "background.svg"
         MouseArea { anchors.fill: parent; onClicked: toggle() }
	 height: gui.height/6; width: height*2.4;
     }

     Image {
         id: knob; source: "knob.svg"; x: 1; y: 2
	 height: gui.height/6; width: gui.height/6;

         MouseArea {
             anchors.fill: parent
             drag.target: knob; drag.axis: "XAxis"; drag.minimumX: 1; drag.maximumX: background.width-knob.width
	     hoverEnabled: false; onReleased: toggle()
         }
     }

     states: [
         State {
             name: "on"
             PropertyChanges { target: knob; x: background.width-knob.width }
	     PropertyChanges { target: toggleswitch; stOn: "true" }
	     StateChangeScript { script: toggleswitch.changed(); }
         },
         State {
             name: "off"
             PropertyChanges { target: knob; x: 1 }
	     PropertyChanges { target: toggleswitch; stOn: "false" }
	     StateChangeScript { script: toggleswitch.changed(); }
         }
     ]

     transitions: Transition {
         NumberAnimation { properties: "x"; easing.type: "InOutQuad"; duration: 200 }
     }
 }
