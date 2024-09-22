import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.configuration 1.0

Item {
    property alias label: slider.label
    property alias configKey: config.key
    property alias defaultValue: config.defaultValue

    height: slider.height
    width: parent.width

    readonly property var _fontSizeConfigValues: [
        "tiny",
        "extraSmall",
        "small",
        "medium",
        "large",
        "extraLarge",
        "huge"
    ]

    readonly property var _fontPixesSizes: [
        Theme.fontSizeTiny,
        Theme.fontSizeExtraSmall,
        Theme.fontSizeSmall,
        Theme.fontSizeMedium,
        Theme.fontSizeLarge,
        Theme.fontSizeExtraLarge,
        Theme.fontSizeHuge
    ]

    readonly property int _defaultIndex: _fontSizeConfigValues.indexOf(defaultValue)

    Slider {
        id: slider

        readonly property real _sliderBarVerticalCenter: Math.round(height - Theme.fontSizeSmall - Theme.paddingSmall - Theme.itemSizeExtraSmall*3/8)
        readonly property color _highlightColor: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor

        function fontSizeIndex(value) {
            var i = _fontSizeConfigValues.indexOf(value)
            return i >= 0 ? i : _defaultIndex
        }

        rightMargin: Theme.paddingLarge
        anchors {
            left: parent.left
            right: sample.left
        }
        stepSize: 1
        minimumValue: 0
        maximumValue: _fontSizeConfigValues.length - 1
        value: fontSizeIndex(config.value)
        valueText: ""
        onSliderValueChanged: config.value = _fontSizeConfigValues[sliderValue]
    }

    MouseArea {
        id: sample

        property bool _highlighted: pressed && containsMouse
        readonly property bool _showPress: _highlighted || pressTimer.running

        width: Theme.itemSizeLarge + Theme.horizontalPageMargin
        height: slider.height
        anchors.right: parent.right

        onPressedChanged: {
            if (pressed) {
                pressTimer.start()
            }
        }

        onCanceled: pressTimer.stop()

        onClicked: slider.value = _defaultIndex

        Text {
            y: slider._sliderBarVerticalCenter - Math.round(height/2)
            anchors {
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
            }
            color: parent._showPress ? Theme.highlightColor : Theme.primaryColor
            font.pixelSize: _fontPixesSizes[slider.sliderValue]
            text: "abc"
        }

        Timer {
            id: pressTimer

            interval: ('minimumPressHighlightTime' in Theme) ? Theme.minimumPressHighlightTime : 64
        }
    }

    ConfigurationValue {
        id: config
    }
}
