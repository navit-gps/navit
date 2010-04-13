
import Qt 4.6

Rectangle {
    id: listselector
    width: parent.width; height: parent.height
    color: "Black"


    property string text: "ListSelector"
    property string value: ""
    property int itemId
    property bool showScroller: false
    signal changed

    function startup() {
        if (listselector.showScroller == true) {
		listScroller.opacity=0.5;
	}
    }
    
    Component.onCompleted: startup();    

    Text {
	id: labelTxt; text: listselector.text; color: "White"; font.pointSize: 14;
	anchors.horizontalCenter: list.horizontalCenter
	anchors.verticalCenter: listselector.top
    }

	ListView {
	     id: list;
             width: listselector.width*0.8; height: listselector.height
	     anchors.top: labelTxt.bottom;
	     anchors.left: listselector.left
             model: listModel
             delegate: listDelegate
             highlight: listHighlight
             focus: true
	     clip: true
	     highlightFollowsCurrentItem: true
	     keyNavigationWraps: true
	     overShoot: false
	     currentIndex: listselector.itemId;
         }
	 Rectangle {
       		 id: listScroller
		opacity: 0; anchors.left: list.right; anchors.leftMargin: 4; width: 6
		y: (list.visibleArea.yPosition * list.height)+(list.visibleArea.heightRatio * list.height/4)
		height: list.visibleArea.heightRatio * list.height
	}
}
