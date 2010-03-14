import Qt 4.6

Rectangle {
    id: slider
    width: background.width + label.width + 40; height: label.height
    color: "Black"

    property int minValue: 0
    property int value: 50
    property int maxValue: 100
    property string text: "Slider"
    signal changed

    function toSlider(inputVal) {
    	return 2+(((inputVal-minValue)/(maxValue-minValue))*126);
    }
    function fromSlider(inputVal) {
    	return minValue+(((inputVal-2)/(126))*(maxValue-minValue));
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
	x: 20; width: 160; height: 16
	gradient: Gradient {
		GradientStop { position: 0.0; color: "steelblue" }
		GradientStop { position: 1.0; color: "lightsteelblue" }
	}
	MouseRegion {
		anchors.fill: parent
		onReleased: { knob.x=mouse.x-15; slider.value=Math.round(fromSlider(knob.x)); slider.changed(); } 
	}

	radius: 8; opacity: 0.7
	Rectangle {
	    id: knob
	    x: 2; y: 2; width: 30; height: 12
	    radius: 6
            gradient: Gradient {
		GradientStop { position: 0.0; color: "lightgray" }
		GradientStop { position: 1.0; color: "gray" }
            }
	    MouseRegion {
		anchors.fill: parent
		drag.target: parent; drag.axis: "XAxis"; drag.minimumX: 2; drag.maximumX: 128
		onPositionChanged: slider.value=Math.round(fromSlider(knob.x))
		onReleased: slider.changed()
	   }
        }
        Text {
            id: valueTxt; text: slider.value; color: "White"; font.pointSize: 14;
            anchors.horizontalCenter: knob.horizontalCenter;
            anchors.verticalCenter: knob.verticalCenter
        }
    }


    Text {
        id: label; text: slider.text; color: "White"; font.pointSize: 18;
        anchors.left: background.right; anchors.leftMargin: 32;
        anchors.verticalCenter: background.verticalCenter
     }
}
