import Qt 4.7
import "pagenavigation.js" as Navit

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

    Behavior on opacity {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

     Component {
         id: listDelegate
         Item {
             id: wrapper
             width: list.width; height: 20
             Column {
                 x: 5; y: 5
                 Text { id: txtItemName; text: itemName; color: "White" }
		 Text { id: txtItemDist; text: itemDistance; color: "White"; anchors.leftMargin: 5; anchors.left: txtItemName.right;anchors.top: txtItemName.top }
		 Text { id: txtItemDirect; text: itemDirection; color: "White"; anchors.leftMargin: 5; anchors.left: txtItemDist.right;anchors.top: txtItemDist.top }
             }
	     MouseArea {
	   		id:delegateMouse
			anchors.fill: parent
			onClicked: { list.currentIndex=itemId; listselector.value=itemValue; listselector.changed() }
	     }
         }
     }

    CommonHighlight { id: listHighlight}

    ListSelector {
	id:layoutList; text: "Current layout"; itemId: navit.getAttrList("layout"); onChanged: navit.setObjectByName("layout",layoutList.value)
	anchors.top: parent.top;
	anchors.left: parent.left;
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
	    id: fullscreenSw; stOn: gui.getAttr("fullscreen");  text: "Fullscreen"; onChanged: gui.setAttr("fullscreen",fullscreenSw.stOn)
        }
        ToggleSwitch {
	    id: tripledSw; stOn: page.isTripleD();  text: "2D/3D"; onChanged: setTripleD(tripledSw.stOn)
        }
        Slider {
	    id: pitchSlider; minValue: 5; maxValue: 90; value: navit.getAttr("pitch"); text: "Pitch"; onChanged: { navit.setAttr("pitch",pitchSlider.value); gui.setAttr("pitch",pitchSlider.value) }
	    opacity: 0
       }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
