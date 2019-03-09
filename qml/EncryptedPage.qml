import QtQuick 2.2
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0
import org.nemomobile.notifications 1.0

Page {
    id: page

    property var hints
    property var foilModel

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
                    mainPage: page
                    foilModel: page.foilModel
                }
            }
            Behavior on opacity { FadeAnimator {} }
        }

        // GeneratingKeyView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilNotesModel.FoilGeneratingKey ||
                        generatingKeyTimer.running) ? 1 : 0
            sourceComponent: Component { GeneratingKeyView {} }
            Behavior on opacity { FadeAnimator {} }
        }

        // EnterPasswordView
        Loader {
            anchors.fill: parent
            active: opacity > 0
            opacity: (foilModel.foilState === FoilNotesModel.FoilLocked ||
                        foilModel.foilState === FoilNotesModel.FoilLockedTimedOut) ? 1 : 0
            sourceComponent: Component {
                EnterPasswordView {
                    mainPage: page
                    foilModel: page.foilModel
                }
            }
            Behavior on opacity { FadeAnimator {} }
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
                }
            }
            Behavior on opacity { FadeAnimator {} }
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
            Behavior on opacity { FadeAnimator {} }
        }
    }
}
