import Qt 4.6
import org.webkit 1.0

Rectangle {
    id: page

    width: gui.width; height: gui.height
    color: "Black"
    opacity: 0

    function pageOpen() {
        page.opacity = 1;
    }
    
    Component.onCompleted: pageOpen();    
    
    opacity: Behavior {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

    Text {
	id: infoTxt;
    	text: point.coordString; color: "White"; font.pointSize: gui.height/20;
	anchors.top:parent.top;anchors.horizontalCenter:parent.horizontalCenter
    }
    Text {
	id: nameTxt;
    	text: point.pointName; color: "White"; font.pointSize: gui.height/20;
	anchors.top:infoTxt.bottom;anchors.topMargin: 5;anchors.horizontalCenter:parent.horizontalCenter
    }
    Text {
	id: typeTxt;
    	text: point.pointType; color: "White"; font.pointSize: gui.height/20;
	anchors.top:nameTxt.bottom;anchors.topMargin: 5;anchors.horizontalCenter:parent.horizontalCenter
    }
    Text {
	id: urlTxt;
    	text: point.pointUrl; color: "Blue"; font.pointSize: gui.height/20;font.underline: true;
	anchors.top:typeTxt.bottom;anchors.topMargin: 5;anchors.horizontalCenter:parent.horizontalCenter
    }

	XmlListModel {
		id: listModel
		xml: point.getInformation();
		query: "/point/element()"
		XmlRole { name: "itemName"; query: "name()" }
		XmlRole { name: "itemDistance"; query: "string()" }
	}

    ListSelector { 
	id:layoutList; text: "Attributes"; itemId: "-1";
	anchors.top: urlTxt.bottom;
	anchors.left: parent.left; anchors.leftMargin: 3
	anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
	width: page.width; height: page.height*0.25
    }
    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
