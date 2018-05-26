import Qt 4.7
import "pagenavigation.js" as Navit

Rectangle {
    id: page

    property string poiFilter: ""

    width: gui.width; height: gui.height
    color: "Black"
    opacity: 0

    function calculateFilter() {
        page.poiFilter='/points/point[type="point_begin" or ';
	if (bankBtn.state=="on") {
		page.poiFilter+='type="poi_bank" or ';
	}
	if (fuelBtn.state=="on") {
		page.poiFilter+='type="poi_fuel" or ';
		page.poiFilter+='type="poi_autoservice" or ';
		page.poiFilter+='type="poi_repair_service" or ';
	}
	if (hotelBtn.state=="on") {
		page.poiFilter+='type="poi_hotel" or ';
		page.poiFilter+='type="poi_camp_rv" or ';
		page.poiFilter+='type="poi_camping" or ';
		page.poiFilter+='type="poi_resort" or ';
		page.poiFilter+='type="poi_motel" or ';
		page.poiFilter+='type="poi_hostel" or ';
	}
	if (foodBtn.state=="on") {
		page.poiFilter+='type="poi_bar" or ';
		page.poiFilter+='type="poi_picnic" or ';
		page.poiFilter+='type="poi_burgerking" or ';
		page.poiFilter+='type="poi_fastfood" or ';
		page.poiFilter+='type="poi_restaurant" or ';
	}
	if (shopBtn.state=="on") {
		page.poiFilter+='type="poi_shop_grocery" or ';
		page.poiFilter+='type="poi_mall" or ';
	}
	if (serviceBtn.state=="on") {
		page.poiFilter+='type="poi_marina" or ';
		page.poiFilter+='type="poi_hospital" or ';
		page.poiFilter+='type="poi_public_utilities" or ';
		page.poiFilter+='type="poi_police" or ';
		page.poiFilter+='type="poi_information" or ';
		page.poiFilter+='type="poi_personal_service" or ';
		page.poiFilter+='type="poi_restroom" or ';
	}
	if (parkingBtn.state=="on") {
		page.poiFilter+='type="poi_car_parking" or ';
	}
	if (landBtn.state=="on") {
		page.poiFilter+='type="poi_land_feature" or ';
		page.poiFilter+='type="poi_rock" or ';
		page.poiFilter+='type="poi_dam" or ';
	}
	page.poiFilter+='type="point_end"]';
	listModel.query=page.poiFilter;
	listModel.reload();
    }

    function pageOpen() {
        page.opacity = 1;
    }

    Component.onCompleted: pageOpen();

    Behavior on opacity {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

    Text {
	id: labelTxt; text: "Points of interest"; color: "White"; font.pointSize: 14;
	anchors.horizontalCenter: page.horizontalCenter
	anchors.top: page.top
    }

  VisualItemModel {
      	id: selectorsModel

        ToggleButton {	id: bankBtn; text: "Bank"; icon: "bank.svg";stOn: "true"; onChanged: calculateFilter(); }
	ToggleButton {	id: fuelBtn; text: "Fuel"; icon: "fuel.svg";stOn: "true"; onChanged: calculateFilter(); }
	ToggleButton {	id: hotelBtn; text: "Hotel"; icon: "bar.svg";stOn: "true"; onChanged: calculateFilter(); }
	ToggleButton {	id: foodBtn; text: "Food"; icon: "fastfood.svg";stOn: "true"; onChanged: calculateFilter(); }
	ToggleButton {	id: shopBtn; text: "Shop"; icon: "shopping.svg";stOn: "true"; onChanged: calculateFilter(); }
	ToggleButton {	id: serviceBtn; text: "Service"; icon: "hospital.svg";stOn: "true"; onChanged: calculateFilter(); }
	ToggleButton {	id: parkingBtn; text: "Parking"; icon: "police.svg";stOn: "true";  onChanged: calculateFilter(); }
	ToggleButton {	id: landBtn; text: "Land features"; icon: "peak.svg";stOn: "true";  onChanged: calculateFilter(); }
  }

  ListView {
      	id: selectorsList; model: selectorsModel
        focus: true; clip: true; orientation: Qt.Horizontal
	anchors.horizontalCenter: page.horizontalCenter; anchors.top: labelTxt.bottom
	width: page.width; height: bankBtn.height*2
  }

	Slider {
	    id: distanceSlider; minValue: 1; maxValue: 250; value: gui.getAttr("radius"); text: "Distance"; onChanged: { gui.setAttr("radius",distanceSlider.value); listModel.xml=point.getPOI("points"); listModel.reload(); }
	    anchors.top: selectorsList.bottom; anchors.horizontalCenter: page.horizontalCenter;
       }

	XmlListModel {
		id: listModel
		xml: point.getPOI("points")
		query: "/points/point"
		XmlRole { name: "itemName"; query: "name/string()" }
		XmlRole { name: "itemType"; query: "type/string()" }
		XmlRole { name: "itemDistance"; query: "distance/string()" }
		XmlRole { name: "itemDirection"; query: "direction/string()" }
		XmlRole { name: "itemValue"; query: "coords/string()" }
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
			onClicked: { point.setNewPoint(itemValue); gui.returnSource="/main.qml"; Navit.load("PageNavigate.qml"); }
	     }
         }
     }

    Component {
        id: listHighlight
        Rectangle {
	    opacity: 0
        }
    }

    ListSelector {
	id:layoutList; text: ""
	anchors.top: distanceSlider.bottom;
	anchors.left: parent.left;
	anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
	width: page.width; height: page.height*0.25
    }

    Cellar {id: cellar; anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
