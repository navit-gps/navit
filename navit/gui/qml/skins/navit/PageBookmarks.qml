import Qt 4.6

Rectangle {
    id: page

    width: gui.width; height: gui.height
    color: "Black"
    opacity: 0

    function bookmarkClick(itemName,itemType,itemCoord) {
        if (itemType=="bookmark_folder") {
	    bookmarks.moveDown(itemName);
	    listModel.xml=bookmarks.getBookmarks("bookmarks");
	    listModel.xml="";
	    listModel.reload();
	}
    }

    function pageOpen() {
        page.opacity = 1;
    }
    
    Component.onCompleted: pageOpen();    
    
    opacity: Behavior {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

    XmlListModel {
	id: listModel
	xml: bookmarks.getBookmarks("bookmarks")
	query: "/bookmarks/bookmark"
	XmlRole { name: "itemName"; query: "label/string()" }
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
                 Text { id: txtItem; text: itemName; color: {itemType== "bookmark" ? "White"  : "Yellow"}
		 font.bold: {itemType!= "bookmark" }
       	         width: list.width-imgDelete.width
		     MouseRegion {
		         id:delegateMouse
			 anchors.fill: parent
			 onClicked: bookmarkClick(itemName,itemType,itemValue);
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
        id: listHighlight
        Rectangle {
	    opacity: 0
        }
    }

    ListSelector { 
	id:layoutList; text: ""
	anchors.top: parent.top;
	anchors.left: parent.left; anchors.leftMargin: 3
	anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
	width: page.width; height: page.height*0.25
    }

    Cellar {id: cellar; anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
