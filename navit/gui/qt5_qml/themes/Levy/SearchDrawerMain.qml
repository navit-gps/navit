import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3


Item {
    id: __root
    property int boxRadius : 10

    signal menuItemClicked(var index)

    QtObject {
        id:navitPlaces
        property var favourites: [
                "5462 Cursus Rd. Walsall E72 0SL United Kingdom",
                "8202 Senectus Av. Brecon R5 1VC United Kingdom",
                "776-4520 Donec Street Milton Keynes P2T 9HV United Kingdom",
                "189-893 Amet St. Newtonmore G4X 9WS United Kingdom",
                "2154 Vel, Street Peterborough Q33 9HT United Kingdom",
                "Ap #819-2987 Cursus Street Milnathort BP5E 9CV United Kingdom",
                "7821 Suspendisse St. Castletown E80 6QU United Kingdom",
                "5904 Sed Avenue Penicuik TP9J 4UK United Kingdom",
                "8713 Vitae St. Banchory JF3 7OO United Kingdom",
                "849-8034 Phasellus St. Felixstowe W96 9VZ United Kingdom",
                "673-1184 Fringilla Road Hartlepool C23 3MN United Kingdom",
                "179-927 Diam. Road Banbury G6 2HP United Kingdom",
                "4041 Non St. Wimborne Minster YI7N 1AA United Kingdom",
                "3855 Convallis St. Lochranza M6 7MG United Kingdom",
                "530-2361 Lorem, Avenue Redruth D3 3PF United Kingdom",
                "863-5481 Vulputate, Road Pontypridd H3 1HZ United Kingdom",
                "5507 Ut Road St. Andrews W3M 5XL United Kingdom",
                "693-9635 Ipsum Ave Tobermory ZU26 5EH United Kingdom",
                "781-1212 Dapibus Rd. Helmsdale GM9K 7FK United Kingdom",
                "102-1859 Lorem Avenue Southend V6 5IQ United Kingdom"
        ]
        property var recents: [
            "539-2120 Lacus. Avenue Northampton ID1 5XG United Kingdom",
            "649-2696 Arcu. Street Hartlepool WU1 0CL United Kingdom",
            "1239 Odio Ave Lochranza XB9L 7JC United Kingdom",
            "9722 Dui. Avenue Kington GD2N 2XH United Kingdom",
            "309-9175 Pede St. Millport EQ1O 0FA United Kingdom",
            "4455 Nunc, Rd. Ellon NL67 1OK United Kingdom",
            "7176 Pede Road Tain I9 8SF United Kingdom",
            "310-4519 Nulla Avenue Llanidloes G3K 7ZK United Kingdom",
            "6786 Sagittis Road Margate C49 3FP United Kingdom",
            "6456 Ut Av. Melton Mowbray ZH4 5NW United Kingdom",
            "974-2380 Facilisis Rd. Grimsby YV29 6OT United Kingdom",
            "5643 Enim. St. Marlborough O7N 1JU United Kingdom",
            "798-6627 Urna. Ave Inverurie BY43 7NS United Kingdom",
            "2248 Ipsum St. Southampton AV21 6AW United Kingdom",
            "499-7436 Nulla Rd. Kingussie V5P 3VS United Kingdom",
            "4426 Accumsan Av. Matlock I7 8TM United Kingdom",
            "2829 Orci. St. March NR9Q 7TQ United Kingdom",
            "1081 Nonummy Road Armadale M17 5AV United Kingdom",
            "913-5445 Mus. Avenue Eastbourne FZ2 1UQ United Kingdom",
            "375-2819 Aliquet. Ave Moffat V3 7WI United Kingdom",
            "8962 Adipiscing Avenue Neath A0 3IH United Kingdom",
            "9384 Sed, Street New Galloway B4K 1HP United Kingdom",
            "3522 Feugiat Street Stoke-on-Trent S7Z 0FB United Kingdom",
            "6383 Aliquam Rd. Kincardine HX8F 0EJ United Kingdom"
        ]
    }

    TabBar {
        id: tabBar
        height: parent.height * 0.1
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        background: Rectangle {
            color: "#00ffffff"
            }

        TabButton {
            id: tabButton
            text: qsTr("Recent")
            font.pointSize: 12
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.bottom: parent.bottom
            background:  Rectangle {
                color: tabBar.currentIndex === 0 ? "#ffffff" : "#a4a4a4"
                implicitWidth: 100
                implicitHeight: 40
                radius: __root.boxRadius
                anchors.left: parent.left
                anchors.right: parent.right
                height: parent.height + radius
            }
            onCheckedChanged: {
                if(checked){
                    if(locationListLoader !== null) {
                        locationListLoader.sourceComponent = placesList
                        locationListLoader.item.model = navitPlaces.recents
                    }
                }
            }
        }

        TabButton {
            id: tabButton1
            text: qsTr("Favourites")
            font.pointSize: 12
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.bottom: parent.bottom
            background: Rectangle {
                color: tabBar.currentIndex === 1 ? "#ffffff" : "#a4a4a4"
                implicitWidth: 100
                implicitHeight: 40
                radius: __root.boxRadius
                anchors.left: parent.left
                anchors.right: parent.right
                height: parent.height + radius
            }
            onCheckedChanged: {
                if(checked){
                    if(typeof(locationListLoader.status) != "null") {
                        locationListLoader.sourceComponent = placesList
                        locationListLoader.item.model = navitPlaces.favourites
                    }

                }
            }
        }
    }

    Item {
        clip: true
        anchors.top: tabBar.bottom
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        Rectangle {
            color: "#ffffff"
            radius: __root.boxRadius
            anchors.fill: parent

            Rectangle {
                x: tabBar.currentIndex === 1 ? width : 0
                width: parent.width / 2
                color: "#ffffff"
                anchors.bottomMargin: parent.radius
                anchors.bottom: parent.bottom
                anchors.top: parent.top
                anchors.topMargin: 0
            }
        }
        Loader {
            id:locationListLoader
            anchors.fill: parent
            anchors.topMargin: parent.height * 0.05
            anchors.bottomMargin: parent.height * 0.05
            anchors.leftMargin: parent.width * 0.05
            anchors.rightMargin: parent.width * 0.05
            sourceComponent: placesList
        }
    }

    Component {
        id:placesList
        ListView {
            id:listView
            clip: true
            model: navitPlaces.recents
            anchors.fill: parent
            delegate: Item {
                id: element
                width: 80
                height: 40


                Text {
                    y: 0
                    text: modelData
                    anchors.left: parent.left
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    onClicked: __root.menuItemClicked(name, action)
                }
            }
        }
    }
}



/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/
