import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

ApplicationWindow {
    id: appWindow

    property bool appEncryptedPagelSelected

    // Reference column width: 960 / 4
    readonly property real _portraitWidth: Math.min(Screen.width, Screen.height)
    readonly property real _landscapeWidth: Math.max(Screen.width, Screen.height)
    readonly property int _portraitCount: Math.floor(_portraitWidth/(Theme.pixelRatio * 240))
    readonly property int _refCellSize: Math.floor(_portraitWidth/_portraitCount)

    // Global properties
    readonly property string appCurrentText: (appEncryptedPagelSelected &&
        FoilNotesModel.foilState === FoilNotesModel.FoilNotesReady) ?
        FoilNotesModel.text : FoilNotesPlaintextModel.text

    readonly property int appPortraitColumnCount: Math.floor(_portraitWidth/_refCellSize)
    readonly property int appLandscapeColumnCount: Math.ceil(_landscapeWidth/_refCellSize)
    readonly property int appPortraitCellSize: Math.floor(_portraitWidth/appPortraitColumnCount)
    readonly property int appLandscapeCellSize: Math.floor(_landscapeWidth/appLandscapeColumnCount)

    signal newNoteFromCover()

    Timer {
        id: lockTimer

        interval: FoilNotesSettings.autoLockTime
        onTriggered: FoilNotesModel.lock(true);
    }

    Connections {
        target: HarbourSystemState
        property bool wasDimmed

        onDisplayStatusChanged: {
            if (target.displayStatus === HarbourSystemState.MCE_DISPLAY_DIM) {
                wasDimmed = true
            } else if (target.displayStatus === HarbourSystemState.MCE_DISPLAY_ON) {
                wasDimmed = false
            }
        }

        onLockedChanged: {
            lockTimer.stop()
            if (target.locked) {
                if (wasDimmed) {
                    // Give the user some time to wake wake up the screen
                    // and prevent encrypted pictures from being locked
                    lockTimer.start()
                } else {
                    FoilNotesModel.lock(false);
                }
            }
        }
    }

    //: Placeholder name for note filename
    //% "note"
    readonly property string appDefaultFileName: qsTrId("foilnotes-default_file_name")

    Component.onCompleted: {
        // Let plaintext model know when encryption is finished:
        FoilNotesModel.encryptionDone.connect(FoilNotesPlaintextModel.onEncryptionDone)
        pageStack.pushAttached(plaintextPageComponent)
    }

    Component {
        id: encryptedPageComponent

        EncryptedPage {
            allowedOrientations: appWindow.allowedOrientations
            hints: FoilNotesHints
            foilModel: FoilNotesModel
            Component.onCompleted: appEncryptedPagelSelected = isCurrentPage
            onIsCurrentPageChanged: appEncryptedPagelSelected = isCurrentPage
            onDecryptNote: FoilNotesPlaintextModel.saveNote(note.pagenr, note.color, note.body)
        }
    }

    Component {
        id: plaintextPageComponent

        PlaintextPage {
            id: plaintextPage

            allowedOrientations: appWindow.allowedOrientations
            hints: FoilNotesHints
            foilModel: FoilNotesModel
            plaintextModel: FoilNotesPlaintextModel

            Connections {
                target: appWindow
                onNewNoteFromCover: {
                    var firstPage = pageStack.currentPage
                    var prevPage = pageStack.previousPage(firstPage)
                    while (prevPage) {
                        firstPage = prevPage
                        prevPage = pageStack.previousPage(prevPage)
                    }
                    pageStack.pop(firstPage, PageStackAction.Immediate)
                    pageStack.navigateForward(PageStackAction.Immediate)
                    plaintextPage.newNoteFromCover()
                    activate()
                }
            }
        }
    }

    initialPage: encryptedPageComponent

    cover: Component {
        CoverPage {
            onNewNote: appWindow.newNoteFromCover()
        }
    }
}
