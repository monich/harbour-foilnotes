import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

ApplicationWindow {
    id: appWindow
    allowedOrientations: appAllowedOrientations

    property bool appEncryptedPagelSelected

    // Reference column width: 960 / 4
    readonly property real _portraitWidth: Math.min(Screen.width, Screen.height)
    readonly property real _landscapeWidth: Math.max(Screen.width, Screen.height)
    readonly property int _portraitCount: Math.floor(_portraitWidth/(Theme.pixelRatio * 240))
    readonly property int _refCellSize: Math.floor(_portraitWidth/_portraitCount)

    // Global properties
    readonly property string appCurrentText: (appEncryptedPagelSelected &&
        FoilNotesModel.foilState === FoilNotesModel.FoilNotesReady) ?
        FoilNotesModel.text : appPlaintextModel.text

    readonly property int appPortraitColumnCount: Math.floor(_portraitWidth/_refCellSize)
    readonly property int appLandscapeColumnCount: Math.ceil(_landscapeWidth/_refCellSize)
    readonly property int appPortraitCellSize: Math.floor(_portraitWidth/appPortraitColumnCount)
    readonly property int appLandscapeCellSize: Math.floor(_landscapeWidth/appLandscapeColumnCount)
    readonly property int appAllowedOrientations: Orientation.All
    readonly property bool appLandscapeMode: orientation === Qt.LandscapeOrientation ||
        orientation === Qt.InvertedLandscapeOrientation
    readonly property int appColumnCount: appLandscapeMode ? appLandscapeColumnCount : appPortraitColumnCount
    readonly property int appCellSize: appLandscapeMode ? appLandscapeCellSize : appPortraitCellSize

    signal newPageFromCover()

    FoilNotesPlaintextModel {
        id: appPlaintextModel
    }

    Connections {
        target: HarbourSystemState
        onLockModeChanged: {
            if (target.locked) {
                FoilNotesModel.lock(false);
            }
        }
    }

    //: Placeholder name for note filename
    //% "note"
    readonly property string appDefaultFileName: qsTrId("foilnotes-default_file_name")

    Component.onCompleted: {
        // Let plaintext model know when encryption is finished:
        FoilNotesModel.encryptionDone.connect(appPlaintextModel.onEncryptionDone)
        pageStack.pushAttached(plaintextPageComponent)
    }

    Component {
        id: encryptedPageComponent

        EncryptedPage {
            allowedOrientations: appAllowedOrientations
            hints: FoilNotesHints
            foilModel: FoilNotesModel
            Component.onCompleted: appEncryptedPagelSelected = isCurrentPage
            onIsCurrentPageChanged: appEncryptedPagelSelected = isCurrentPage
            onDecryptNote: appPlaintextModel.saveNote(note.pagenr, note.color, note.body)
        }
    }

    Component {
        id: plaintextPageComponent

        PlaintextPage {
            id: plaintextPage

            allowedOrientations: appAllowedOrientations
            hints: FoilNotesHints
            foilModel: FoilNotesModel
            plaintextModel: appPlaintextModel

            Connections {
                target: appWindow
                onNewPageFromCover: {
                    pageStack.pop(null, PageStackAction.Immediate)
                    pageStack.navigateForward(PageStackAction.Immediate)
                    plaintextPage.newNote(PageStackAction.Immediate)
                    activate()
                }
            }
        }
    }

    initialPage: encryptedPageComponent

    cover: Component {
        CoverPage {
            onNewNote: appWindow.newPageFromCover()
        }
    }
}
