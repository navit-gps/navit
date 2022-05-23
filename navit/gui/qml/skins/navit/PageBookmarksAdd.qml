import Qt 4.7
import "pagenavigation.js" as Navit

Rectangle {
    id: page

    width: gui.width; height: gui.height
    color: "Black"
    opacity: 0

    Timer {
	id: backTimer
        interval: 2000;
        onTriggered: Navit.back();
    }

    function add(description) {
    	resultTxt.text=bookmarks.AddBookmark(description);
	btnOk.opacity=0;
	btnCancel.opacity=0;
	resultTxt.opacity=1;
	backTimer.start()
    }

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

   TextInput{
     	id: searchTxt; text: "Enter bookmark name..."
	anchors.top: nameTxt.bottom;anchors.topMargin: 5;anchors.horizontalCenter:parent.horizontalCenter
	width: page.width; font.pointSize: 14; color: "White";focus: true; readOnly: false
   }


	ButtonIcon {
            id: btnOk; text: "Add"; icon: "gui_active.svg"; onClicked: add(searchTxt.text)
	    anchors.top: searchTxt.bottom;anchors.topMargin: 5;anchors.right:parent.horizontalCenter;anchors.rightMargin:10
        }
	ButtonIcon {
            id: btnCancel; text: "Cancel"; icon: "gui_inactive.svg"; onClicked: gui.backToPrevPage();
	    anchors.top: searchTxt.bottom;anchors.topMargin: 5;anchors.left:parent.horizontalCenter;anchors.leftMargin:10
        }

    Text {
	id: resultTxt;
    	text: point.pointName; color: "White"; font.pointSize: gui.height/20;
	anchors.top:btnCancel.bottom;anchors.topMargin: 5;anchors.horizontalCenter:parent.horizontalCenter
	opacity: 0
    }

    Cellar {id: cellar; anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
