import Qt 4.7

Rectangle {
    id: slider
    width: background.width + label.width + gui.width/24; height: label.height
    color: "Black"

    property int minValue: 0
    property int value: 50
    property int maxValue: 100
    property string text: "Slider"
    signal changed

    function toSlider(inputVal) {
    	return 2+(((inputVal-minValue)/(maxValue-minValue))*(background.width-knob.width-2));
    }
    function fromSlider(inputVal) {
    	return minValue+(((inputVal-2)/(background.width-knob.width-2))*(maxValue-minValue));
    }

    function startup () {
        if ( slider.value < slider.minValue ) {
	    slider.value = slider.minValue;
	}
        if ( slider.value > slider.maxValue ) {
	    slider.value = slider.maxValue;
	}
    	knob.x = toSlider(slider.value);
    }

    Component.onCompleted: startup();

    Rectangle {
        id: background
	x: 20; width: gui.width/5; height: gui.height/24
	gradient: Gradient {
		GradientStop { position: 0.0; color: "steelblue" }
		GradientStop { position: 1.0; color: "lightsteelblue" }
	}
	MouseArea {
		anchors.fill: parent
		onReleased: { knob.x=mouse.x-15; slider.value=Math.round(fromSlider(knob.x)); slider.changed(); }
	}

	radius: 8; opacity: 0.7
	Rectangle {
	    id: knob
	    x: 2; y: 2; width: gui.width/26; height: gui.height/35
	    radius: 6
            gradient: Gradient {
		GradientStop { position: 0.0; color: "lightgray" }
		GradientStop { position: 1.0; color: "gray" }
            }
	    MouseArea {
		anchors.fill: parent
		drag.target: parent; drag.axis: "XAxis"; drag.minimumX: 2; drag.maximumX: background.width-knob.width
		onPositionChanged: slider.value=Math.round(fromSlider(knob.x))
		onReleased: slider.changed()
	   }
        }
        Text {
            id: valueTxt; text: slider.value; color: "White"; font.pointSize: gui.height/32;
            anchors.horizontalCenter: knob.horizontalCenter;
            anchors.verticalCenter: knob.verticalCenter
        }
    }


    Text {
        id: label; text: slider.text; color: "White"; font.pointSize: gui.height/24;
        anchors.left: background.right; anchors.leftMargin: gui.width/24;
        anchors.verticalCenter: background.verticalCenter
     }
}
