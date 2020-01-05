import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

Page {
    id: page

    property var foilModel

    allowedOrientations: Orientation.All

    Connections {
        target: page.foilModel
        onFoilStateChanged: {
            if (foilModel.foilState !== FoilNotesModel.FoilNotesReady) {
                page.backNavigation = false
                pageStack.pop(pageStack.previousPage(page))
            }
        }
    }

    GenerateKeyView {
        anchors.fill: parent
        //: Prompt label
        //% "You are about to generate a new key"
        prompt: qsTrId("foilnotes-generate_key_page-title")
        foilModel: page.foilModel
    }
}
