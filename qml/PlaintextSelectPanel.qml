import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Item {
    id: panel
    height: Math.max(deleteButton.height, encryptButton.height) + 2 * Theme.paddingMedium
    y: parent.height - visiblePart
    visible: visiblePart > 0

    property bool active
    property real visiblePart: active ? height : 0
    property bool canDelete: true
    property bool canEncrypt: true

    signal deleteSelected()
    signal deleteHint()
    signal encryptSelected()
    signal encryptHint()

    anchors {
        left: parent.left
        right: parent.right
    }

    HarbourIconTextButton {
        id: deleteButton
        // Position it in the middle when encryptButton is hidden
        x: Math.floor((canEncrypt ? panel.width/4 : panel.width/2) - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        iconSource: "image://theme/icon-m-delete"
        enabled: canDelete
        onClicked: panel.deleteSelected()
        onPressAndHold: panel.deleteHint()
    }

    HarbourIconTextButton {
        id: encryptButton
        x: Math.floor(3*panel.width/4 - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        iconSource: "image://theme/icon-m-device-lock"
        visible: canEncrypt
        onClicked: panel.encryptSelected()
        onPressAndHold: panel.encryptHint()
    }

    Behavior on visiblePart { SmoothedAnimation { duration: 250  } }
}
