import Qt 4.6

Rectangle {
    id: page

    width: gui.width; height: gui.height
    border.width: 1
    color: "Black"
    opacity: 0

    function setTripleD(flag) {
    	if ( flag=="true" ) {
	    navit.setAttr("pitch",gui.getAttr("pitch"));
	    pitchSlider.value = navit.getAttr("pitch");
	    pitchSlider.startup();
	    pitchSlider.opacity = 1
	} else {
	    navit.setAttr("pitch","0");
	    pitchSlider.opacity = 0
	}
    }

    function isTripleD() {
        if ( navit.getAttr("pitch") == "0" )
	    return "0";
	else
	    return "1";
    }

    function pageOpen() {
        if ( isTripleD() == "1" ) {
	    pitchSlider.opacity = 1
	}
        page.opacity = 1;
    }
    
    Component.onCompleted: pageOpen();    
    
    opacity: Behavior {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

    ListSelector { 
	id:layoutList; text: "Current layout"; itemId: navit.getAttrList("layout"); onChanged: navit.setObjectByName("layout",layoutList.value)
	anchors.top: parent.top;
	anchors.left: parent.left; anchors.leftMargin: 3
	anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
	width: page.width/2; height: page.height*0.25
    }
    Grid {
        columns: 1; rows: 3
	anchors.right: parent.right
	anchors.top: parent.top;
	anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
        spacing: gui.width/12
        ToggleSwitch {
	    id: fullscreenSw; on: gui.getAttr("fullscreen");  text: "Fullscreen"; onChanged: gui.setAttr("fullscreen",fullscreenSw.on)
        }
        ToggleSwitch {
	    id: tripledSw; on: page.isTripleD();  text: "2D/3D"; onChanged: setTripleD(tripledSw.on)
        }
        Slider {
	    id: pitchSlider; minValue: 5; maxValue: 90; value: navit.getAttr("pitch"); text: "Pitch"; onChanged: { navit.setAttr("pitch",pitchSlider.value); gui.setAttr("pitch",pitchSlider.value) }
	    opacity: 0
       }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
