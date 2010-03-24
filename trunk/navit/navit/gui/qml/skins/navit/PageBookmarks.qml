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

   Rectangle {
    color: "Black"

    id:layoutList
    anchors.top: parent.top; anchors.left: parent.left; anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
    width: page.width; height: page.height/2-cellar.height

     Component {
         id: delegate
         Item {
             id: wrapper
             width: list.width; height: 20
             Column {
                 x: 5; y: 5
                 Text { text: itemName; color: "White" }
             }
	     MouseRegion {
	   		id:delegateMouse
			anchors.fill: parent
			onClicked: bookmarkClick(itemId,itemIcon,itemName,itemPath);
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
	id: labelTxt; text: "Bookmarks"; color: "White"; font.pointSize: 14;
	anchors.horizontalCenter: list.horizontalCenter
	anchors.verticalCenter: layoutList.top
    }

	ListView {
	     id: list;
             width: layoutList.width*0.8; height: layoutList.height
	     anchors.top: labelTxt.bottom;
	     anchors.left: layoutList.left
             model: listModel
             delegate: delegate
             highlight: highlight
             focus: true
	     clip: true
	     highlightFollowsCurrentItem: true
	     keyNavigationWraps: true
	     overShoot: false
	     currentIndex: bookmarks.getAttrList("");
         }
	 Rectangle {
       		 id: listScroller
		opacity: 0.5; anchors.left: list.right; anchors.leftMargin: 4; width: 6
		y: (list.visibleArea.yPosition * list.height)+(list.visibleArea.heightRatio * list.height/4)
		height: list.visibleArea.heightRatio * list.height
	}
    }

    Cellar {id: cellar; anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
