import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0
import org.nemomobile.notifications 1.0

import "foil-ui"

Page {
    id: thisPage

    property var hints
    property var foilModel
    readonly property real screenHeight: isPortrait ? Screen.height : Screen.width
    property bool isCurrentPage: true

    signal decryptNote(var note)

    property var foilUi

    function getFoilUi() {
        if (!foilUi) {
            foilUi = foilUiComponent.createObject(thisPage)
        }
        return foilUi
    }

    Component {
        id: foilUiComponent

        QtObject {
            readonly property real opacityFaint: HarbourTheme.opacityFaint
            readonly property real opacityLow: HarbourTheme.opacityLow
            readonly property real opacityHigh: HarbourTheme.opacityHigh
            readonly property real opacityOverlay: HarbourTheme.opacityOverlay

            readonly property var settings: FoilNotesSettings
            readonly property bool otherFoilAppsInstalled: FoilNotes.otherFoilAppsInstalled
            function isLockedState(foilState) {
                return foilState === FoilNotesModel.FoilLocked ||
                    foilState === FoilNotesModel.FoilLockedTimedOut
            }
            function isReadyState(foilState) {
                return foilState === FoilNotesModel.FoilNotesReady
            }
            function isGeneratingKeyState(foilState) {
                return foilState === FoilNotesModel.FoilGeneratingKey
            }
            function qsTrEnterPasswordViewMenuGenerateNewKey() {
                //: Pulley menu item
                //% "Generate a new key"
                return qsTrId("foilnotes-menu-generate_key")
            }
            function qsTrEnterPasswordViewEnterPasswordLong() {
                //: Password prompt label (long)
                //% "Secret notes are locked. Please enter your password"
                return qsTrId("foilnotes-enter_password_view-label-enter_password")
            }
            function qsTrEnterPasswordViewEnterPasswordShort() {
                //: Password prompt label (short)
                //% "Please enter your password"
                return qsTrId("foilnotes-enter_password_view-label-enter_password_short")
            }
            function qsTrEnterPasswordViewButtonUnlock() {
                //: Button label
                //% "Unlock"
                return qsTrId("foilnotes-enter_password_view-button-unlock")
            }
            function qsTrEnterPasswordViewButtonUnlocking() {
                //: Button label
                //% "Unlocking..."
                return qsTrId("foilnotes-enter_password_view-button-unlocking")
            }
            function qsTrAppsWarningText() {
                //: Warning text, small size label below the password prompt
                //% "Note that all Foil apps share the encryption key and the password."
                return qsTrId("foilnotes-foil_apps_warning")
            }
            function qsTrGenerateKeyWarningTitle() {
                //: Title for the new key warning
                //% "Warning"
                return qsTrId("foilnotes-generate_key_warning-title")
            }
            function qsTrGenerateKeyWarningText() {
                //: Warning shown prior to generating the new key
                //% "Once you have generated a new key, you are going to lose access to all the files encrypted by the old key. Note that the same key is used by all Foil apps, such as Foil Auth and Foil Pics. If you have forgotten your password, then keep in mind that most likely it's computationally easier to brute-force your password and recover the old key than to decrypt files for which the key is lost."
                return qsTrId("foilnotes-generate_key_warning-text")
            }
            function qsTrGenerateNewKeyPrompt() {
                //: Prompt label
                //% "You are about to generate a new key"
                return qsTrId("foilnotes-generate_key_page-title")
            }
            function qsTrGenerateKeySizeLabel() {
                //: Combo box label
                //% "Key size"
                return qsTrId("foilnotes-generate_key_view-label-key_size")
            }
            function qsTrGenerateKeyPasswordDescription(minLen) {
                //: Password field label
                //% "Type at least %0 character(s)"
                return qsTrId("foilnotes-generate_key_view-label-minimum_length",minLen).arg(minLen)
            }
            function qsTrGenerateKeyButtonGenerate() {
                //: Button label
                //% "Generate key"
                return qsTrId("foilnotes-generate_key_view-button-generate_key")
            }
            function qsTrGenerateKeyButtonGenerating() {
                //: Button label
                //% "Generating..."
                return qsTrId("foilnotes-generate_key_view-button-generating_key")
            }
            function qsTrConfirmPasswordPrompt() {
                //: Password confirmation label
                //% "Please type in your new password one more time"
                return qsTrId("foilnotes-confirm_password_page-info_label")
            }
            function qsTrConfirmPasswordDescription() {
                //: Password confirmation description
                //% "Make sure you don't forget your password. It's impossible to either recover it or to access the encrypted notes without knowing it. Better take it seriously."
                return qsTrId("foilnotes-confirm_password_page-description")
            }
            function qsTrConfirmPasswordRepeatPlaceholder() {
                //: Placeholder for the password confirmation prompt
                //% "New password again"
                return qsTrId("foilnotes-confirm_password_page-text_field_placeholder-new_password")
            }
            function qsTrConfirmPasswordRepeatLabel() {
                //: Label for the password confirmation prompt
                //% "New password"
                return qsTrId("foilnotes-confirm_password_page-text_field_label-new_password")
            }
            function qsTrConfirmPasswordButton() {
                //: Button label (confirm password)
                //% "Confirm"
                return qsTrId("foilnotes-confirm_password_page-button-confirm")
            }
        }
    }

    Connections {
        target: foilModel
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

    Component {
        id: foilNotesIconComponent

        Rectangle {
            id: circle

            width: Theme.itemSizeHuge
            height: width
            color: Theme.rgba(Theme.primaryColor, HarbourTheme.opacityFaint * HarbourTheme.opacityLow)
            radius: width/2

            Image {
                source: HarbourTheme.darkOnLight ? "images/fancy-lock-dark.svg" : "images/fancy-lock.svg"
                height: Math.floor(circle.height * 5 / 8)
                sourceSize.height: height
                anchors.centerIn: circle
                visible: parent.opacity > 0
            }
        }
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
                FoilUiGenerateKeyView {
                    foilUi: getFoilUi()
                    foilModel: thisPage.foilModel
                    page: thisPage
                    //: Label text
                    //% "You need to generate the key and select the password before you can encrypt your notes"
                    prompt: qsTrId("foilnotes-generate_key_view-label-key_needed")
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
                FoilUiGeneratingKeyView {
                    //: Progress view label
                    //% "Generating new key..."
                    text: qsTrId("foilnotes-generating_key_view-generating_new_key")
                    isLandscape: thisPage.isLandscape
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
                FoilUiEnterPasswordView {
                    foilUi: getFoilUi()
                    foilModel: thisPage.foilModel
                    page: thisPage
                    iconComponent: foilNotesIconComponent
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
                    foilModel: thisPage.foilModel
                    isLandscape: thisPage.isLandscape
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

                    mainPage: thisPage
                    hints: thisPage.hints
                    foilUi: getFoilUi()
                    foilModel: thisPage.foilModel
                    pulleyFlickable: flickable
                    onDecryptNote: thisPage.decryptNote(note)
                    Connections {
                        target: thisPage
                        onIsCurrentPageChanged: {
                            if (thisPage.isCurrentPage) {
                                encryptedNotesView.searchMode = false
                            }
                        }
                    }
                }
            }
            onReadyChanged: {
                if (!ready && thisPage.isCurrentPage) {
                    pageStack.pop(thisPage, PageStackAction.Immediate)
                }
            }
            Behavior on opacity { FadeAnimation { } }
        }
    }
}
