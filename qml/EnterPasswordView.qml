import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

import "harbour"

Item {
    id: view

    property var foilModel
    property bool isLandscape
    property bool orientationTransitionRunning
    property bool wrongPassword

    readonly property bool landscapeLayout: isLandscape && Screen.sizeCategory < Screen.Large
    readonly property bool unlocking: foilModel.foilState !== FoilNotesModel.FoilLocked &&
                                    foilModel.foilState !== FoilNotesModel.FoilLockedTimedOut
    readonly property bool canEnterPassword: inputField.text.length > 0 && !unlocking &&
                                    !wrongPasswordAnimation.running && !wrongPassword

    function enterPassword() {
        if (!foilModel.unlock(inputField.text)) {
            wrongPassword = true
            wrongPasswordAnimation.start()
            inputField.requestFocus()
        }
    }

    PullDownMenu {
        id: pullDownMenu

        MenuItem {
            //: Pulley menu item
            //% "Generate a new key"
            text: qsTrId("foilnotes-menu-generate_key")
            onClicked: {
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
            }
        }
    }

    Rectangle {
        id: circle

        anchors.horizontalCenter: parent.horizontalCenter
        y: (panel.y > height) ? Math.floor((panel.y - height)/2) : (panel.y - height)
        width: Theme.itemSizeHuge
        height: width
        color: Theme.rgba(Theme.primaryColor, HarbourTheme.opacityFaint * HarbourTheme.opacityLow)
        radius: width/2
        visible: opacity > 0 && !orientationTransitionRunning

        // Hide it when it's only partially visible (i.e. in langscape)
        // or getting too close to the edge of the screen
        opacity: (y < Theme.paddingMedium) ? 0 : 1
        Behavior on opacity {
            enabled: !orientationTransitionRunning
            FadeAnimation { duration: 100 }
        }

        Image {
            source: HarbourTheme.darkOnLight ? "images/fancy-lock-dark.svg" : "images/fancy-lock.svg"
            height: Math.floor(circle.height * 5 / 8)
            sourceSize.height: height
            anchors.centerIn: circle
            visible: parent.opacity > 0
        }
    }

    Item {
        id: panel

        width: parent.width
        height: childrenRect.height
        y: (parent.height > height) ? Math.floor((parent.height - height)/2) : (parent.height - height)

        readonly property bool showLongPrompt: y >= Theme.paddingMedium

        InfoLabel {
            id: longPrompt

            visible: panel.showLongPrompt
            //: Password prompt label (long)
            //% "Secret notes are locked. Please enter your password"
            text: qsTrId("foilnotes-enter_password_view-label-enter_password")
        }

        InfoLabel {
            anchors.bottom: longPrompt.bottom
            visible: !panel.showLongPrompt
            //: Password prompt label (short)
            //% "Please enter your password"
            text: qsTrId("foilnotes-enter_password_view-label-enter_password_short")
        }

        HarbourPasswordInputField {
            id: inputField

            anchors {
                left: panel.left
                top: longPrompt.bottom
                topMargin: Theme.paddingLarge
            }
            enabled: !unlocking
            onTextChanged: view.wrongPassword = false
            EnterKey.onClicked: view.enterPassword()
            EnterKey.enabled: view.canEnterPassword
        }

        Button {
            id: button

            text: unlocking ?
                //: Button label
                //% "Unlocking..."
                qsTrId("foilnotes-enter_password_view-button-unlocking") :
                //: Button label
                //% "Unlock"
                qsTrId("foilnotes-enter_password_view-button-unlock")
            enabled: view.canEnterPassword
            onClicked: view.enterPassword()
        }
    }

    HarbourShakeAnimation  {
        id: wrongPasswordAnimation

        target: panel
    }

    Loader {
        anchors {
            top: parent.top
            topMargin: screenHeight - height - Theme.paddingLarge
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
        }
        readonly property bool display: FoilNotesSettings.sharedKeyWarning && FoilNotes.otherFoilAppsInstalled
        opacity: display ? 1 : 0
        active: opacity > 0
        sourceComponent: Component {
            FoilAppsWarning {
                onClicked: FoilNotesSettings.sharedKeyWarning = false
            }
        }
        Behavior on opacity { FadeAnimation {} }
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
                    anchors.rightMargin: 0
                },
                AnchorChanges {
                    target: button
                    anchors {
                        top: inputField.bottom
                        horizontalCenter: parent.horizontalCenter
                    }
                },
                PropertyChanges {
                    target: button
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
                    target: inputField
                    anchors.right: button.left
                },
                PropertyChanges {
                    target: inputField
                    anchors.rightMargin: Theme.horizontalPageMargin
                },
                AnchorChanges {
                    target: button
                    anchors {
                        top: longPrompt.bottom
                        right: panel.right
                        horizontalCenter: undefined
                    }
                },
                PropertyChanges {
                    target: button
                    anchors {
                        topMargin: Theme.paddingLarge
                        rightMargin: Theme.horizontalPageMargin
                    }
                }
            ]
        }
    ]
}
