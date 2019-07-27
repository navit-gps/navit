import QtQuick 2.9
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

Item {
    id: __root
    signal resultClicked()
    Connections {
        target: backend
        onSearchResultsChanged : {
            console.log(JSON.stringify(backend.searchresults))
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: backend.searchresults
        delegate: Item {
            x: 5
            width: 80
            height: 40
            MouseArea {
                anchors.fill: parent
                onClicked: __root.resultClicked()
            }

            Text {
                text: name
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

}
