import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Item {
    id: panel
    height: Math.max(deleteButton.height, decryptButton.height) + 2 * Theme.paddingMedium
    y: parent.height - visiblePart
    visible: visiblePart > 0

    property bool active
    property real visiblePart: active ? height : 0
    property bool canDelete: deleteButton.enabled
    property bool canDecrypt: decryptButton.enabled

    signal deleteSelected()
    signal deleteHint()
    signal decryptSelected()
    signal decryptHint()

    anchors {
        left: parent.left
        right: parent.right
    }

    HarbourIconTextButton {
        id: deleteButton
        x: Math.floor(panel.width/4 - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        iconSource: "image://theme/icon-m-delete"
        onClicked: panel.deleteSelected()
        onPressAndHold: panel.deleteHint()
    }

    HarbourIconTextButton {
        id: decryptButton
        x: Math.floor(3*panel.width/4 - width/2)
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        iconSource: "image://theme/icon-m-note"
        onClicked: panel.decryptSelected()
        onPressAndHold: panel.decryptHint()
    }

    Behavior on visiblePart { SmoothedAnimation { duration: 250  } }
}
