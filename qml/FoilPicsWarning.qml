import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

Item {
    x: Theme.horizontalPageMargin
    y: ((appLandscapeMode ? Screen.width : Screen.height) + _yAttachTo - height)/2
    width: parent.width - 2 * x
    height: Theme.itemSizeSmall
    opacity: 0.6
    visible: FoilNotes.foilPicsInstalled

    property Item topPanel
    readonly property real _yAttachTo: topPanel ? (topPanel.y + topPanel.height) : 0

    Image {
        id: foilpicsIcon

        opacity: HarbourTheme.darkOnLight ? 0.6 : 0.4
        source: "images/harbour-foilpics.svg"
        width: size
        height: size
        sourceSize: Qt.size(size, size)
        anchors.verticalCenter: parent.verticalCenter
        readonly property real size: Math.floor(Theme.itemSizeSmall/2)
    }

    Label {
        anchors {
            leftMargin: Theme.horizontalPageMargin
            left: foilpicsIcon.right
            right: parent.right
            verticalCenter: parent.verticalCenter
        }
        font.pixelSize: Theme.fontSizeExtraSmall
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignLeft
        color: Theme.highlightColor
        //: Warning text, small size label below the password prompt
        //% "Note that Foil Notes and Foil Pics share the encryption key and the password."
        text: qsTrId("foilnotes-foil_pics_warning")
    }
}
