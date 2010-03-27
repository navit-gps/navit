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
                 Text { id: txtItem; text: itemName; color: "White" 
       			width: list.width-imgDelete.width
		MouseRegion {
			id:delegateMouse
			anchors.fill: parent
			onClicked: bookmarkClick(itemId,itemIcon,itemName,itemPath);
		}

		 }
		 Image {
			id: imgCut; source: gui.iconPath+"zoom_out.svg"; anchors.right: imgCopy.left;anchors.rightMargin: 5;
			width: 20; height: 20;

			MouseRegion {
				id:delegateMouseCut
				anchors.fill: parent
				onClicked: { bookmarks.Cut(itemId); bookmarks.getAttrList(""); }
			}
		}
		 Image {
			id: imgCopy; source: gui.iconPath+"zoom_in.svg"; anchors.right: imgPaste.left;anchors.rightMargin: 5;
			width: 20; height: 20;

			MouseRegion {
				id:delegateMouseCopy
				anchors.fill: parent
				onClicked: { bookmarks.Copy(itemId); bookmarks.getAttrList(""); }
			}
		}
		Image {
			id: imgPaste; source: gui.iconPath+"mark.svg"; anchors.right: imgDelete.left;anchors.rightMargin: 5;
			width: 20; height: 20;

			MouseRegion {
				id:delegateMousePaste
				anchors.fill: parent
				onClicked: { bookmarks.Paste(bookmarks.currentPath); bookmarks.getAttrList(""); }
			}
		}
		 Image {
			id: imgDelete; source: gui.iconPath+"gui_inactive.svg"; anchors.right: wrapper.right;anchors.rightMargin: 5;
			width: 20; height: 20;

			MouseRegion {
				id:delegateMouseDelete
				anchors.fill: parent
				onClicked: { bookmarks.Delete(itemId); bookmarks.getAttrList(""); }
			}
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
