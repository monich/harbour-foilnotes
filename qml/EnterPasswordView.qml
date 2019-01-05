import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0
import "harbour"

SilicaFlickable {
    id: view
    property var foilModel
    property bool wrongPassword
    readonly property bool landscapeLayout: appLandscapeMode && Screen.sizeCategory < Screen.Large
    readonly property bool unlocking: foilModel.foilState !== FoilNotesModel.FoilLocked &&
                                    foilModel.foilState !== FoilNotesModel.FoilLockedTimedOut

    function enterPassword() {
        if (!foilModel.unlock(passphraseField.text)) {
            wrongPassword = true
            wrongPasswordAnimation.start()
            passphraseField.requestFocus()
        }
    }

    PullDownMenu {
        id: pullDownMenu
        MenuItem {
            //: Pulley menu item
            //% "Generate a new key"
            text: qsTrId("foilnotes-menu-generate_key")
            onClicked: {
                if (foilModel.mayHaveEncryptedNotes) {
                    var warning = pageStack.push(Qt.resolvedUrl("GenerateKeyWarning.qml"));
                    warning.accepted.connect(function() {
                        // Replace the warning page with a slide. This may
                        // occasionally generate "Warning: cannot pop while
                        // transition is in progress" if the user taps the
                        // page stack indicator (as opposed to the Accept
                        // button) but this warning is fairly harmless:
                        //
                        // _dialogDone (Dialog.qml:124)
                        // on_NavigationChanged (Dialog.qml:177)
                        // navigateForward (PageStack.qml:291)
                        // onClicked (private/PageStackIndicator.qml:174)
                        //
                        warning.canNavigateForward = false
                        pageStack.replace(Qt.resolvedUrl("GenerateKeyPage.qml"), {
                            foilModel: foilModel
                        })
                    })
                } else {
                    pageStack.push(Qt.resolvedUrl("GenerateKeyPage.qml"), {
                        foilModel: foilModel
                    })
                }
            }
        }
    }

    Item {
        id: panel

        width: parent.width
        height: childrenRect.height
        anchors.verticalCenter: parent.verticalCenter

        InfoLabel {
            id: prompt

            height: implicitHeight
            //: Password prompt label
            //% "Secret notes are locked. Please enter your password"
            text: qsTrId("foilnotes-enter_password_view-label-enter_password")
        }

        HarbourPasswordInputField {
            id: passphraseField

            anchors {
                left: panel.left
                top: prompt.bottom
                topMargin: Theme.paddingLarge
            }
            enabled: !unlocking
            EnterKey.onClicked: view.enterPassword()
            onTextChanged: view.wrongPassword = false
        }

        Button {
            id: unlockButton

            text: unlocking ?
                //: Button label
                //% "Unlocking..."
                qsTrId("foilnotes-enter_password_view-button-unlocking") :
                //: Button label
                //% "Unlock"
                qsTrId("foilnotes-enter_password_view-button-unlock")
            enabled: passphraseField.text.length > 0 && !unlocking && !wrongPasswordAnimation.running && !wrongPassword
            onClicked: view.enterPassword()
        }
    }

    FoilPicsWarning {
        topPanel: panel
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
                    target: passphraseField
                    anchors.right: panel.right
                },
                PropertyChanges {
                    target: passphraseField
                    anchors.rightMargin: 0
                },
                AnchorChanges {
                    target: unlockButton
                    anchors {
                        top: passphraseField.bottom
                        horizontalCenter: parent.horizontalCenter
                    }
                },
                PropertyChanges {
                    target: unlockButton
                    anchors {
                        topMargin: Theme.paddingLarge
                        rightMargin: 0
                    }
                }
            ]
        },
        State {
            name: "landscape"
            when: landscapeLayout
            changes: [
                AnchorChanges {
                    target: passphraseField
                    anchors.right: unlockButton.left
                },
                PropertyChanges {
                    target: passphraseField
                    anchors.rightMargin: Theme.horizontalPageMargin
                },
                AnchorChanges {
                    target: unlockButton
                    anchors {
                        top: prompt.bottom
                        right: panel.right
                        horizontalCenter: undefined
                    }
                },
                PropertyChanges {
                    target: unlockButton
                    anchors {
                        topMargin: Theme.paddingLarge
                        rightMargin: Theme.horizontalPageMargin
                    }
                }
            ]
        }
    ]
}
