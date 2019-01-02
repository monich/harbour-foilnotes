import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property alias foilModel: page.foilModel

    allowedOrientations: Orientation.All

    GenerateKeyView {
        anchors.fill: parent
        //: Label text
        //% "You are about to generate a new key"
        title: qsTrId("foilnotes-generate_key_page-title")
        foilModel: page.foilModel
    }
}
