import QtQuick 2.0
import Sailfish.Silica 1.0
import QtGraphicalEffects 1.0

import "harbour"

Item {
    readonly property real jailSize: Math.min(parent.width, parent.height) - 2 * Theme.paddingLarge

    Image {
        readonly property real size: Theme.itemSizeHuge
        anchors.centerIn: parent
        sourceSize.height: size
        source: "images/harbour-foilnotes.svg"
    }

    Image {
        id: shadow

        anchors.centerIn: parent
        sourceSize.height: jailSize
        source: "images/jail-black.svg"
        visible: false
    }

    FastBlur {
        source: shadow
        anchors.fill: shadow
        radius: 16
        transparentBorder: true
    }

    HarbourHighlightIcon {
        id: jail

        anchors.centerIn: parent
        sourceSize.height: jailSize
        highlightColor: Theme.secondaryColor
        source: "images/jail.svg"
    }
}
