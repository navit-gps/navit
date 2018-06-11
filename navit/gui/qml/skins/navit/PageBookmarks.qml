import Qt 4.7
import "pagenavigation.js" as Navit

Rectangle {
    id: page

    width: gui.width; height: gui.height
    color: "Black"
    opacity: 0

    function bookmarkReload() {
	    listModel.xml=bookmarks.getBookmarks();
	    listModel.query="/bookmarks/bookmark";
	    listModel.reload();
    }

    function bookmarkClick(itemName,itemType,itemCoord) {
        if (itemType=="bookmark_folder") {
	    if ( itemName==".." ) {
		bookmarks.moveUp();
	    } else {
	        bookmarks.moveDown(itemName);
	    }
	    bookmarkReload();
	} else {
	   bookmarks.setPoint(itemName);
	   pageLoader.source="PageNavigate.qml";
	}
    }

    function pageOpen() {
        page.opacity = 1;
    }

    Component.onCompleted: pageOpen();

    Behavior on opacity {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

    ButtonIcon { id: folderBtn; text: "New folder"; icon: "gui_active.svg"; onClicked: folderDialog.opacity=1
		      anchors.top: parent.top; anchors.topMargin: page.height/16; anchors.left: parent.left; anchors.leftMargin: page.width/16
    }
    ButtonIcon { id: pasteBtn; text: "Paste"; icon: "gui_active.svg"; onClicked: { bookmarks.Paste(); bookmarkReload(); }
		      anchors.top: parent.top; anchors.topMargin: page.height/16; anchors.left: folderBtn.right; anchors.leftMargin: page.width/16
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
		     MouseArea {
		         id:delegateMouse
			 anchors.fill: parent
			 onClicked: bookmarkClick(itemName,itemType,itemValue);
		     }
		 }
		 Image {
			id: imgCut; source: gui.iconPath+"zoom_out.svg"; anchors.right: imgCopy.left;anchors.rightMargin: 5;
			width: 20; height: 20;

			MouseArea {
				id:delegateMouseCut
				anchors.fill: parent
				onClicked: { bookmarks.Cut(itemName); bookmarkReload(); }
			}
		}
		 Image {
			id: imgCopy; source: gui.iconPath+"zoom_in.svg"; anchors.right: imgDelete.left;anchors.rightMargin: 5;
			width: 20; height: 20;

			MouseArea {
				id:delegateMouseCopy
				anchors.fill: parent
				onClicked: { bookmarks.Copy(itemName); bookmarkReload(); }
			}
		}
    	       Image {
			id: imgDelete; source: gui.iconPath+"gui_inactive.svg"; anchors.right: wrapper.right;anchors.rightMargin: 5;
			width: 20; height: 20;

			MouseArea {
				id:delegateMouseDelete
				anchors.fill: parent
				onClicked: { bookmarks.Delete(itemName); bookmarkReload(); }
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
	anchors.top: pasteBtn.bottom;
	anchors.left: parent.left;
	anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
	width: page.width; height: page.height*0.25
    }

    Cellar {id: cellar; anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }

    Rectangle {
        id: folderDialog
        opacity:0
	anchors.horizontalCenter: page.horizontalCenter; anchors.verticalCenter: page.verticalCenter;
	width: page.width; height:page.height/3
        color: "Grey";

	TextInput{
     	    id: folderTxt; text: "Enter folder name here..."
	    anchors.top: parent.top;anchors.topMargin: 5;anchors.horizontalCenter:parent.horizontalCenter
	    width: page.width; font.pointSize: 14; color: "White";focus: true; readOnly: false
       }

	ButtonIcon {
            id: btnOk; text: "Add"; icon: "gui_active.svg"; onClicked: { bookmarks.AddFolder(folderTxt.text); bookmarkReload(); folderDialog.opacity=0 }
	    anchors.top: folderTxt.bottom;anchors.topMargin: 5;anchors.right:parent.horizontalCenter;anchors.rightMargin:10
        }
	ButtonIcon {
            id: btnCancel; text: "Cancel"; icon: "gui_inactive.svg"; onClicked: folderDialog.opacity=0;
	    anchors.top: folderTxt.bottom;anchors.topMargin: 5;anchors.left:parent.horizontalCenter;anchors.leftMargin:10
        }
    }
}
