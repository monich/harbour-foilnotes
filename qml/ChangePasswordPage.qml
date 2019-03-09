import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

import "harbour"

Page {
    id: page

    allowedOrientations: window.allowedOrientations

    property Page mainPage
    property var foilModel
    property bool wrongPassword
    property alias currentPassword: currentPasswordInput.text
    property alias newPassword: newPasswordInput.text

    function canChangePassword() {
        return currentPassword.length > 0 && newPassword.length > 0 && currentPassword !== newPassword && !wrongPassword
    }

    function invalidPassword() {
        wrongPassword = true
        wrongPasswordAnimation.start()
        currentPasswordInput.requestFocus()
    }

    function changePassword() {
        if (canChangePassword()) {
            if (foilModel.checkPassword(currentPassword)) {
                pageStack.push(Qt.resolvedUrl("ConfirmPasswordPage.qml"), {
                    password: newPassword
                }).passwordConfirmed.connect(function() {
                    if (foilModel.changePassword(currentPassword, newPassword)) {
                        pageStack.pop(mainPage)
                    } else {
                        invalidPassword()
                    }
                })
            } else {
                invalidPassword()
            }
        }
    }

    onStatusChanged: {
        if (status === PageStatus.Activating) {
            currentPasswordInput.requestFocus()
        }
    }

    Column {
        id: column

        width: parent.width
        spacing: Theme.paddingLarge
        anchors.verticalCenter: parent.verticalCenter

        InfoLabel {
            //: Password change prompt
            //% "Please enter the current and the new password"
            text: qsTrId("foilnotes-change_password_page-label-enter_passwords")
        }
        HarbourPasswordInputField {
            id: currentPasswordInput

            //: Placeholder and label for the current password prompt
            //% "Current password"
            label: qsTrId("foilnotes-change_password_page-text_field_label-current_password")
            placeholderText: label
            EnterKey.onClicked: newPasswordInput.focus = true
            onTextChanged: page.wrongPassword = false
        }
        HarbourPasswordInputField {
            id: newPasswordInput

            //: Placeholder and label for the new password prompt
            //% "New password"
            placeholderText: qsTrId("foilnotes-change_password_page-text_field_label-new_password")
            label: placeholderText
            EnterKey.onClicked: page.changePassword()
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            //: Button label
            //% "Change password"
            text: qsTrId("foilnotes-change_password_page-button-change_password")
            enabled: canChangePassword()
            onClicked: page.changePassword()
        }
    }

    FoilPicsWarning {
        topPanel: column
    }

    HarbourShakeAnimation  {
        id: wrongPasswordAnimation

        target: column
    }
}
