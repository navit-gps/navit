import Qt 4.7
import "pagenavigation.js" as Navit

Rectangle {
    id: page

    width: gui.width; height: gui.height
    color: "Black"
    opacity: 0

    function pageOpen() {
       if ( search.countryName.length>0 ) {
	    gridCity.opacity = 1;
       }
       if ( search.townName.length>0 ) {
	    gridStreet.opacity = 1;
       }
       if ( search.streetName.length>0 ) {
	    gridAddress.opacity = 0; //Disabled ,because housenamuber search is broken
       }
        page.opacity = 1;
    }

    Component.onCompleted: pageOpen();

    Behavior on opacity {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

    Grid {
        columns: 2;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.verticalCenter; anchors.bottomMargin: gui.height/16;
        spacing: gui.width/12
	Grid {
	    columns: 1;rows: 2;
	    id: gridCountry;
            Text {
                id: txtItemCountry; text: "Country";
	        color: "White"; font.pointSize: gui.height/32; horizontalAlignment: Qt.AlignHCenter
	    }
            ButtonIcon {
                id: btnCountry; text: search.countryName; icon: "country_"+search.countryISO2+".svgz"; onClicked: { search.searchContext="country"; Navit.load("PageSearchSelector.qml"); }
            }
	}
	Grid {
	    columns: 1; rows: 2;
	    id: gridCity
	    opacity: 0;
            Text {
                id: txtItemCity; text: "Town";
	        color: "White"; font.pointSize: gui.height/32; horizontalAlignment: Qt.AlignHCenter
	    }
            ButtonIcon {
                id: btnCity; text: search.townName; icon: "gui_bookmark.svg"; onClicked: { search.searchContext="town"; Navit.load("PageSearchSelector.qml"); }
            }
	}
    }

    Grid {
        columns: 2;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.verticalCenter; anchors.topMargin: gui.height/16;
        spacing: gui.width/12
	Grid {
	    columns: 1; rows: 2;
	    id: gridStreet
	    opacity: 0;
            Text {
                id: txtItemStreet; text: "Street";
	        color: "White"; font.pointSize: gui.height/32; horizontalAlignment: Qt.AlignHCenter
	    }
            ButtonIcon {
                id: btnStreet; text: search.streetName; icon: "gui_town.svg"; onClicked: { search.searchContext="street"; Navit.load("PageSearchSelector.qml"); }
            }
       }
       Grid {
           columns: 1; rows: 2;
	   id: gridAddress;
	   opacity: 0;
            Text {
                id: txtItemAddress; text: "Address";
	        color: "White"; font.pointSize: gui.height/32; horizontalAlignment: Qt.AlignHCenter
	    }
            ButtonIcon {
                id: btnAddress; text: "Address"; icon: "attraction.svg"; onClicked: console.log("Implement me!");
            }
	}
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
