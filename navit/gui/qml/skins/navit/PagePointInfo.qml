import Qt 4.7
import org.webkit 1.0
import "pagenavigation.js" as Navit

Rectangle {
    id: page

    width: gui.width; height: gui.height
    color: "Black"
    opacity: 0

    function pageOpen() {
        page.opacity = 1;
    }

    Component.onCompleted: pageOpen();

    Behavior on opacity {
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
	XmlRole { name: "itemType"; query: "name()" }
	XmlRole { name: "itemAttribute"; query: "string()" }
    }

    Component {
        id: listDelegate
        Item {
            id: wrapper
            width: list.width; height: 20
            Column {
                x: 5; y: 5
                Text { id: txtItemType; text: itemType; color: "White"; font.bold: true; }
	 Text { id: txtItemAttribute; text: itemAttribute; color: "White"; anchors.leftMargin: 5; anchors.left: txtItemType.right;anchors.top: txtItemType.top }
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
	id:layoutList; text: "Attributes";
	anchors.top: urlTxt.bottom;
	anchors.left: parent.left;
	anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
	width: page.width; height: page.height*0.25
    }
    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
