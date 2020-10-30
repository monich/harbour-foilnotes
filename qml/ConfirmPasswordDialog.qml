import QtQuick 2.0
import Sailfish.Silica 1.0

import "harbour"

Dialog {
    id: dialog

    allowedOrientations: window.allowedOrientations
    forwardNavigation: false

    property string password
    property bool wrongPassword

    readonly property bool landscapeLayout: isLandscape && Screen.sizeCategory < Screen.Large
    readonly property bool canCheckPassword: inputField.text.length > 0 &&
                                             inputField.text.length > 0 && !wrongPassword

    signal passwordConfirmed()

    function checkPassword() {
        if (inputField.text === password) {
            dialog.passwordConfirmed()
        } else {
            wrongPassword = true
            wrongPasswordAnimation.start()
            inputField.requestFocus()
        }
    }

    onStatusChanged: {
        if (status === PageStatus.Activating) {
            inputField.requestFocus()
        }
    }

    // Otherwise width is changing with a delay, causing visible layout changes
    onIsLandscapeChanged: width = isLandscape ? Screen.height : Screen.width

    InfoLabel {
        //: Password confirmation label
        //% "Please type in your new password one more time"
        text: qsTrId("foilnotes-confirm_password_page-info_label")

        // Bind to panel x position for shake animation
        x: Theme.horizontalPageMargin + panel.x
        width: parent.width - 2 * Theme.horizontalPageMargin
        anchors {
            bottom: panel.top
            bottomMargin: Theme.paddingLarge
        }

        // Hide it when it's only partially visible
        opacity: (y < Theme.paddingSmall) ? 0 : 1
        Behavior on opacity {
            enabled: !orientationTransitionRunning
            FadeAnimation { }
        }
    }

    Item {
        id: panel

        width: parent.width
        height: childrenRect.height
        y: (parent.height > height) ? Math.floor((parent.height - height)/2) : (parent.height - height)

        Label {
            id: warning

            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x

            //: Password confirmation description
            //% "Make sure you don't forget your password. It's impossible to either recover it or to access the encrypted notes without knowing it. Better take it seriously."
            text: qsTrId("foilnotes-confirm_password_page-description")
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            wrapMode: Text.Wrap
        }

        HarbourPasswordInputField {
            id: inputField

            anchors {
                left: panel.left
                top: warning.bottom
                topMargin: Theme.paddingLarge
            }

            //: Placeholder for the password confirmation prompt
            //% "New password again"
            placeholderText: qsTrId("foilnotes-confirm_password_page-text_field_placeholder-new_password")
            //: Label for the password confirmation prompt
            //% "New password"
            label: qsTrId("foilnotes-confirm_password_page-text_field_label-new_password")
            onTextChanged: dialog.wrongPassword = false
            EnterKey.enabled: dialog.canCheckPassword
            EnterKey.onClicked: dialog.checkPassword()
        }

        Button {
            id: button

            anchors {
                topMargin: Theme.paddingLarge
                bottomMargin: 2 * Theme.paddingSmall
            }

            //: Button label (confirm password)
            //% "Confirm"
            text: qsTrId("foilnotes-confirm_password_page-button-confirm")
            enabled: dialog.canCheckPassword
            onClicked: dialog.checkPassword()
        }
    }

    HarbourShakeAnimation  {
        id: wrongPasswordAnimation

        target: panel
    }

    states: [
        State {
            name: "portrait"
            when: !landscapeLayout
            changes: [
                AnchorChanges {
                    target: inputField
                    anchors.right: panel.right
                },
                PropertyChanges {
                    target: inputField
                    anchors {
                        rightMargin: 0
                        bottomMargin: Theme.paddingLarge
                    }
                },
                AnchorChanges {
                    target: button
                    anchors {
                        top: inputField.bottom
                        right: undefined
                        horizontalCenter: parent.horizontalCenter
                        bottom: undefined
                    }
                },
                PropertyChanges {
                    target: button
                    anchors.rightMargin: 0
                }
            ]
        },
        State {
            name: "landscape"
            when: landscapeLayout
            changes: [
                AnchorChanges {
                    target: inputField
                    anchors.right: button.left
                },
                PropertyChanges {
                    target: inputField
                    anchors {
                        rightMargin: Theme.horizontalPageMargin
                        bottomMargin: Theme.paddingSmall
                    }
                },
                AnchorChanges {
                    target: button
                    anchors {
                        top: undefined
                        right: panel.right
                        horizontalCenter: undefined
                        bottom: inputField.bottom
                    }
                },
                PropertyChanges {
                    target: button
                    anchors.rightMargin: Theme.horizontalPageMargin
                }
            ]
        }
    ]
}
