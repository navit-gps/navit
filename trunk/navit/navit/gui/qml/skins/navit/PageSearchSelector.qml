
import Qt 4.6

Rectangle {
    id: page

    width: gui.width; height: gui.height
    color: "Black"
    opacity: 0

    function setSearchResult() {
        if (search.searchContext=="country") {
	    search.countryName=layoutList.value;
	    gui.backToPrevPage();
	}
        if (search.searchContext=="town") {
	    search.townName=layoutList.value;
	    gui.backToPrevPage();
	}
    }

    function pageOpen() {
        page.opacity = 1;
    }
    
    Component.onCompleted: pageOpen();    
    
    opacity: Behavior {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

   TextInput{
     	id: searchTxt; text: search.countryName;
	anchors.top: parent.top; anchors.left: parent.left; anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
	width: page.width; font.pointSize: 14; color: "White";focus: true; readOnly: false
   }

   Component {
         id: listDelegate
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

    ListSelector { 
	id:layoutList; text: search.searchContext; itemId: search.getAttrList(search.searchContext); onChanged: setSearchResult()
	anchors.top: searchTxt.bottom; anchors.left: parent.left; anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
	width: page.width; height: page.height/2-cellar.height
    }

    Cellar {id: cellar; anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
