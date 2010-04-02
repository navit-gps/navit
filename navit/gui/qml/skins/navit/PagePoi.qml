import Qt 4.6

Rectangle {
    id: page

    width: gui.width; height: gui.height
    color: "Black"
    opacity: 0

    function bookmarkClick(itemId,itemIcon,itemName,itemPath) {
	if ( itemIcon=="yes" ) {
	    bookmarks.currentPath=itemPath;
	    bookmarks.getAttrList("");
	} else {
	   bookmarks.setPoint(itemId);
	   gui.setPage("point.qml");
	}
    }

    function pageOpen() {
        page.opacity = 1;
    }
    
    Component.onCompleted: pageOpen();    
    
    opacity: Behavior {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

    Text {
	id: labelTxt; text: "Points of interest"; color: "White"; font.pointSize: 14;
	anchors.horizontalCenter: page.horizontalCenter
	anchors.top: page.top
    }

  VisualItemModel {
      	id: selectorsModel

        ToggleButton {	id: bankBtn; text: "Bank"; icon: "bank.svg";stOn: "true"; }
	ToggleButton {	id: fuelBtn; text: "Fuel"; icon: "fuel.svg";stOn: "true"; }
	ToggleButton {	id: hotelBtn; text: "Hotel"; icon: "bar.svg";stOn: "true"; }
	ToggleButton {	id: foodBtn; text: "Food"; icon: "fastfood.svg";stOn: "true"; }
	ToggleButton {	id: shopBtn; text: "Shop"; icon: "shopping.svg";stOn: "true"; }
	ToggleButton {	id: serviceBtn; text: "Service"; icon: "hospital.svg";stOn: "true"; }
	ToggleButton {	id: parkingBtn; text: "Parking"; icon: "police.svg";stOn: "true"; }
	ToggleButton {	id: landBtn; text: "Land features"; icon: "peak.svg";stOn: "true"; }
  }

  ListView {
      	id: selectorsList; model: selectorsModel
        focus: true; clip: true; orientation: Qt.Horizontal
	anchors.horizontalCenter: page.horizontalCenter; anchors.top: labelTxt.bottom
	width: page.width; height: bankBtn.height*2
  }

	Slider {
	    id: distanceSlider; minValue: 1; maxValue: 250; value: gui.getAttr("radius"); text: "Distance"; onChanged: { gui.setAttr("radius",distanceSlider.value); point.getAttrList("points"); }
	    anchors.top: selectorsList.bottom; anchors.horizontalCenter: page.horizontalCenter;
       }

    ListSelector { 
	id:layoutList; text: ""; itemId: point.getAttrList("points"); onChanged: console.log("Poi clicked");
	anchors.top: distanceSlider.bottom;
	anchors.left: parent.left; anchors.leftMargin: 3
	anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
	width: page.width; height: page.height*0.25
    }

    Cellar {id: cellar; anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
