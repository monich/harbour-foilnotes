import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    id: coloritem

    signal clicked
    property alias number: label.text

    height: Theme.itemSizeExtraSmall
    width: Theme.itemSizeExtraSmall
    radius: Theme.paddingSmall/2
    anchors {
        right: parent.right
        rightMargin: Theme.horizontalPageMargin
        verticalCenter: parent.verticalCenter
        topMargin: Theme.paddingLarge
    }
    Label {
        id: label
        font.pixelSize: Theme.fontSizeLarge
        anchors.centerIn: parent
    }
    MouseArea {
        anchors { fill: parent; margins: -Theme.paddingMedium }
        onClicked: parent.clicked()
    }
}
