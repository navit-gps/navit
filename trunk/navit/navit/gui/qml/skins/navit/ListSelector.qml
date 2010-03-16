
import Qt 4.6

Rectangle {
    id: listselector
    width: labelTxt.width + 40 + 180; height: 60
    color: "Black"


    property string text: "ListSelector"
    property string value: ""
    property string itemId: "0"
    signal changed

 ListModel {
     id: contactModel
     ListElement {
         itemId: 0
         name: "Car"
     }
     ListElement {
	 itemId: 1
         name: "John Brown"
     }
     ListElement {
         itemId: 2
         name: "Sam Wise"
     }
     ListElement {
         itemId: 3
         name: "T@H"
     }
 }
     Component {
         id: delegate
         Item {
             id: wrapper
             width: 180; height: 20
             Column {
                 x: 5; y: 5
                 Text { text: name; color: "White" }
             }
	     MouseRegion {
	   		id:delegateMouse
			anchors.fill: parent
			onClicked: { list.currentIndex=itemId; listselector.value=name; listselector.changed() }
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
             width: 180; height: 60
	     anchors.top: labelTxt.bottom;
	     anchors.left: listselector.left
             model: contactModel
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
		opacity: 0.5; anchors.left: list.right; anchors.leftMargin: 4; width: 6
		y: (list.visibleArea.yPosition * list.height)+(list.visibleArea.heightRatio * list.height/4)
		height: list.visibleArea.heightRatio * list.height
	}
}
