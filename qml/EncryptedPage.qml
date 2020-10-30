import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0
import org.nemomobile.notifications 1.0

Page {
    id: page

    property var hints
    property var foilModel
    readonly property real screenHeight: isPortrait ? Screen.height : Screen.width

    // nextPage is either
    // a) our attached page; or
    // b) the page we pushed to the stack
    readonly property Page nextPage: pageStack.nextPage(page)
    readonly property bool isCurrentPage: status === PageStatus.Active || status === PageStatus.Activating ||
        (nextPage && page.parent && nextPage.parent !== page.parent.attachedContainer)

    signal decryptNote(var note)

    Connections {
        target: page.foilModel
        property int lastFoilState
        onFoilStateChanged: {
            // Don't let the progress screens disappear too fast
            switch (foilModel.foilState) {
            case FoilNotesModel.FoilGeneratingKey:
                generatingKeyTimer.start()
                break
            case FoilNotesModel.FoilDecrypting:
                decryptingTimer.start()
                break
            }
            lastFoilState = target.foilState
        }
        onKeyGenerated: {
            //: Pop-up notification
            //% "Generated new key"
            notification.previewBody = qsTrId("foilnotes-notification-generated_key")
            notification.publish()
        }
        onPasswordChanged: {
            //: Pop-up notification
            //% "Password changed"
            notification.previewBody = qsTrId("foilnotes-notification-password_changed")
            notification.publish()
        }
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            foilModel.textIndex = -1
        }
    }

    // Otherwise width is changing with a delay, causing visible layout changes
    onIsLandscapeChanged: width = isLandscape ? Screen.height : Screen.width

    Notification {
        id: notification

        expireTimeout: 2000
        Component.onCompleted: {
            if ('icon' in notification) {
                notification.icon = "icon-s-certificates"
            }
        }
    }

    Timer {
        id: generatingKeyTimer

        interval: 1000
    }

    Timer {
        id: decryptingTimer

        interval: 1000
    }

    SilicaFlickable {
        id: flickable

        anchors.fill: parent
        contentHeight: height

        // GenerateKeyView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilNotesModel.FoilKeyMissing) ? 1 : 0
            sourceComponent: Component {
                GenerateKeyView {
                    foilModel: page.foilModel
                    isLandscape: page.isLandscape
                }
            }
            Behavior on opacity { FadeAnimation { } }
        }

        // GeneratingKeyView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilNotesModel.FoilGeneratingKey ||
                        generatingKeyTimer.running) ? 1 : 0
            sourceComponent: Component {
                GeneratingKeyView {
                    isLandscape: page.isLandscape
                }
            }
            Behavior on opacity { FadeAnimation { } }
        }

        // EnterPasswordView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilNotesModel.FoilLocked ||
                        foilModel.foilState === FoilNotesModel.FoilLockedTimedOut) ? 1 : 0
            sourceComponent: Component {
                EnterPasswordView {
                    foilModel: page.foilModel
                    isLandscape: page.isLandscape
                    orientationTransitionRunning: page.orientationTransitionRunning
                }
            }
            Behavior on opacity { FadeAnimation { } }
        }

        // DecryptingView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilNotesModel.FoilDecrypting ||
                      decryptingTimer.running) ? 1 : 0
            sourceComponent: Component {
                DecryptingView {
                    foilModel: page.foilModel
                    isLandscape: page.isLandscape
                }
            }
            Behavior on opacity { FadeAnimation { } }
        }

        // EncryptedNotesView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            readonly property bool ready: foilModel.foilState === FoilNotesModel.FoilNotesReady
            opacity: (ready && !generatingKeyTimer.running && !decryptingTimer.running) ? 1 : 0
            sourceComponent: Component {
                EncryptedNotesView {
                    id: encryptedNotesView

                    mainPage: page
                    hints: page.hints
                    foilModel: page.foilModel
                    isLandscape: page.isLandscape
                    pulleyFlickable: flickable
                    onDecryptNote: page.decryptNote(note)
                    Connections {
                        target: page
                        onIsCurrentPageChanged: {
                            if (page.isCurrentPage) {
                                encryptedNotesView.searchMode = false
                            }
                        }
                    }
                }
            }
            onReadyChanged: {
                if (!ready && page.isCurrentPage) {
                    pageStack.pop(page, PageStackAction.Immediate)
                }
            }
            Behavior on opacity { FadeAnimation { } }
        }
    }
}
