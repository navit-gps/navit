
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

    ListSelector { 
	id:layoutList; text: search.searchContext; itemId: search.getAttrList(search.searchContext); onChanged: setSearchResult()
	anchors.top: parent.top; anchors.left: parent.left; anchors.topMargin: gui.height/16; anchors.leftMargin: gui.width/32
	width: page.width; height: page.height/2-cellar.height
    }

    Cellar {id: cellar; anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
