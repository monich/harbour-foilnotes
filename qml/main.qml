import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
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

    //: Placeholder name for note filename
    //% "note"
    readonly property string appDefaultFileName: qsTrId("foilnotes-default_file_name")

    signal newNoteFromCover()

    function popAll() {
        var firstPage = pageStack.currentPage
        var prevPage = pageStack.previousPage(firstPage)
        while (prevPage) {
            firstPage = prevPage
            prevPage = pageStack.previousPage(prevPage)
        }
        pageStack.pop(firstPage, PageStackAction.Immediate)
    }

    function resetAutoLock() {
        lockTimer.stop()
        if (FoilNotesModel.keyAvailable && FoilNotesSettings.autoLock && HarbourSystemState.locked) {
            lockTimer.start()
        }
    }

    HarbourWakeupTimer {
        id: lockTimer

        interval: FoilNotesSettings.autoLockTime
        onTriggered: FoilNotesModel.lock(false);
    }

    Connections {
        target: HarbourSystemState
        onLockedChanged: resetAutoLock()
    }

    Connections {
        target: FoilNotesSettings
        onAutoLockChanged: resetAutoLock()
    }

    Connections {
        target: FoilNotesModel
        onEncryptionDone: FoilNotesPlaintextModel.onEncryptionDone(requestId, success)
        onFoilStateChanged: resetAutoLock()
    }

    Binding {
        target: FoilNotesNfcShareService
        property: "active"
        value: !jailed && NfcSystem.version >= NfcSystem.Version_1_1_0 && Qt.application.active
    }

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
                    if (FoilNotesSettings.plaintextView) {
                        selectPlaintextPage.start()
                    }
                }
            }
            Timer {
                id: selectPlaintextPage
                interval: 0
                onTriggered: pageStack.navigateForward(PageStackAction.Immediate)
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

            onIsCurrentPageChanged: {
                encryptedPageSelected = !isCurrentPage
                FoilNotesSettings.plaintextView = isCurrentPage
            }

            Connections {
                target: appWindow
                onNewNoteFromCover: {
                    popAll()
                    pageStack.navigateForward(PageStackAction.Immediate)
                    plaintextPage.newNoteFromCover()
                    activate()
                }
            }

            Notification {
                id: nfcNoteNotification

                expireTimeout: 2000
                //: Pop-up notification
                //% "Note received via NFC"
                previewBody: qsTrId("foilnotes-notification-nfc_note_received")
                Component.onCompleted: {
                    if ('icon' in nfcNoteNotification) {
                        nfcNoteNotification.icon = "icon-m-nfc"
                    }
                }
            }

            Connections {
                target: FoilNotesNfcShareService
                onNewNote: {
                    popAll()
                    pageStack.navigateForward(PageStackAction.Immediate)
                    FoilNotesPlaintextModel.addNote(color, body)
                    plaintextPage.openFirstNote(color, body, PageStackAction.Immediate)
                    nfcNoteNotification.publish()
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
