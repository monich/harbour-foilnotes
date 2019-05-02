import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

Page {
    id: page

    property string qrcode
    readonly property int actualWidth: isPortrait ? Screen.width : Screen.height
    readonly property int actualHeight: isPortrait ? Screen.height : Screen.width

    Rectangle {
        color: "white"
        x: Math.floor((actualWidth - width)/2)
        y: Math.floor((actualHeight - height)/2)
        width: qrcodeImage.width + 2 * Theme.horizontalPageMargin
        height: qrcodeImage.height + 2 * Theme.horizontalPageMargin

        Image {
            id: qrcodeImage

            asynchronous: true
            anchors.centerIn: parent
            source: page.qrcode ? "image://qrcode/" + page.qrcode : ""
            readonly property int maxDisplaySize: Math.min(Screen.width, Screen.height) - 4 * Theme.horizontalPageMargin
            readonly property int maxSourceSize: Math.max(sourceSize.width, sourceSize.height)
            readonly property int n: Math.floor(maxDisplaySize/maxSourceSize)
            width: sourceSize.width * n
            height: sourceSize.height * n
            smooth: false
        }
    }
}
