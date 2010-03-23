import Qt 4.6

Rectangle {
    id: page

    width: gui.width; height: gui.height
    color: "Black"
    opacity: 0

    function pageOpen() {
        page.opacity = 1;
    }
    
    Component.onCompleted: pageOpen();    
    
    opacity: Behavior {
        NumberAnimation { id: opacityAnimation; duration: 300; alwaysRunToEnd: true }
    }

    Grid {
        columns: 2;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.bottom: parent.verticalCenter; anchors.bottomMargin: gui.height/16;
        spacing: gui.width/12
        ButtonIcon {
            id: btnCountry; text: search.countryName; icon: "country_"+search.countryISO2+".svgz"; onClicked: { search.searchContext="country"; gui.setPage("PageSearchSelector.qml"); }
        }
        ButtonIcon {
            id: btnCity; text: "City"; icon: "gui_bookmark.svg"; onClicked: { search.searchContext="town"; gui.setPage("PageSearchSelector.qml"); }
        }
    }

    Grid {
        columns: 2;rows: 1
        anchors.horizontalCenter: parent.horizontalCenter;
        anchors.top: parent.verticalCenter; anchors.topMargin: gui.height/16;
        spacing: gui.width/12
        ButtonIcon {
            id: btnStreet; text: "Street"; icon: "gui_town.svg"; onClicked: console.log("Implement me!");
        }
        ButtonIcon {
            id: btnAddress; text: "Address"; icon: "attraction.svg"; onClicked: console.log("Implement me!");
        }
    }

    Cellar {anchors.bottom: page.bottom; anchors.horizontalCenter: page.horizontalCenter; width: page.width }
}
