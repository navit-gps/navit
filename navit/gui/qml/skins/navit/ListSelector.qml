
import Qt 4.6

Rectangle {
    id: listselector
    width: parent.width; height: parent.height
    color: "Black"


    property string text: "ListSelector"
    property string value: ""
    property string itemId: "0"
    property bool showScroller: false
    signal changed

    function startup() {
        if (listselector.showScroller == true) {
		listScroller.opacity=0.5;
	}
    }
    
    Component.onCompleted: startup();    

     Component {
         id: delegate
         Item {
             id: wrapper
             width: list.width; height: 20
             Column {
                 x: 5; y: 5
                 Text { id: txtItemName; text: itemName; color: "White" }
		 Text { id: txtItemDist; text: itemDistance; color: "White"; anchors.leftMargin: 5; anchors.left: txtItemName.right;anchors.top: txtItemName.top }
		 Text { id: txtItemDirect; text: itemDirection; color: "White"; anchors.leftMargin: 5; anchors.left: txtItemDist.right;anchors.top: txtItemDist.top }
             }
	     MouseRegion {
	   		id:delegateMouse
			anchors.fill: parent
			onClicked: { list.currentIndex=itemId; listselector.value=itemValue; listselector.changed() }
	     }
         }
     }

     Component {
         id: highlight
         Rectangle {
             color: "lightsteelblue"
             radius: 5
         }
     }

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
             delegate: delegate
             highlight: highlight
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
