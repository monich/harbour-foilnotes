import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

ApplicationWindow {
    id: appWindow

    property bool encryptedPageSelected: true
    readonly property bool jailed: HarbourProcessState.jailedApp

    // Reference column width: 960 / 4
    readonly property real _portraitWidth: Math.min(Screen.width, Screen.height)
    readonly property real _landscapeWidth: Math.max(Screen.width, Screen.height)
    readonly property int _portraitCount: Math.floor(_portraitWidth/(Theme.pixelRatio * 240))
    readonly property int _refCellSize: Math.floor(_portraitWidth/_portraitCount)

    // Global properties
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
        target: FoilNotesModel
        onEncryptionDone: FoilNotesPlaintextModel.onEncryptionDone(requestId, success)
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

    Component {
        id: encryptedPageComponent

        EncryptedPage {
            allowedOrientations: appWindow.allowedOrientations
            isCurrentPage: encryptedPageSelected
            hints: FoilNotesHints
            foilModel: FoilNotesModel
            onDecryptNote: FoilNotesPlaintextModel.saveNote(note.pagenr, note.color, note.body)
            // Attempt to push an attached page in Component.onCompleted results in
            // [W] PageStack.js:89: Error: Cannot pushAttached while operation is in progress: push
            // Let's do it the first time when the initial page becomes active
            onStatusChanged: {
                if (status === PageStatus.Active && !forwardNavigation) {
                    // We have no attached page yet
                    pageStack.pushAttached(plaintextPageComponent)
                }
            }
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
            onIsCurrentPageChanged: encryptedPageSelected = !isCurrentPage

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

    Component {
        id: jailPageComponent

        JailPage {
            allowedOrientations: appWindow.allowedOrientations
        }
    }

    initialPage: jailed ? jailPageComponent : encryptedPageComponent

    cover: Component {
        CoverPage {
            encryptedPageSelected: appWindow.encryptedPageSelected
            onNewNote: appWindow.newNoteFromCover()
        }
    }
}
