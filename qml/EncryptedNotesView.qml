import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

import "harbour"

SilicaFlickable {
    id: thisView

    property Page mainPage
    property var hints
    property var foilUi
    property var foilModel
    property var pulleyFlickable

    property bool searchMode
    readonly property bool busy: foilModel.busy || progressTimer.running
    readonly property bool canNavigate: !grid.contextMenuItem

    readonly property int _columnCount: mainPage.isPortrait ? appPortraitColumnCount : appLandscapeColumnCount

    signal decryptNote(var note)

    function deleteSelectedNotes(model) {
        var rows = model.selectedRows()
        if (rows.length > 0) {
            remorseActionOnSelectedNotes(model,
                //: Generic remorse popup text
                //% "Deleting %0 selected notes"
                qsTrId("foilnotes-remorse-deleting_selected", rows.length).arg(rows.length),
                function() { model.deleteNotes(rows) },
                function() { model.clearSelection() }
            )
        }
    }

    function decryptSelectedNotes(model) {
        var rows = model.selectedRows()
        if (rows.length > 0) {
            remorseActionOnSelectedNotes(model,
                //: Generic remorse popup text
                //% "Decrypting %0 selected notes"
                qsTrId("foilnotes-remorse-decrypting_selected", rows.length).arg(rows.length),
               function() {
                   var rows = foilModel.selectedRows()
                   for (var i = 0; i < rows.length; i++) {
                       thisView.decryptNote(foilModel.get(rows[i]))
                   }
                   foilModel.deleteNotes(rows)
                   leftSwipeToDecryptedHintLoader.armed = true
               },
               function() { model.clearSelection() }
            )
        }
    }

    function remorseActionOnSelectedNotes(model, title, triggerCallback, cancelCallback) {
        pageStack.pop()
        bulkActionRemorse.execute(title, triggerCallback)
        bulkActionRemorse.canceled.connect(cancelCallback)
    }

    Component.onCompleted: pullDownMenu.updateMenuItems()

    onMovementStarted: grid.focus = false   // close the vkb

    Component {
        id: selectPageComponent

        SelectPage {
            id: selectPage

            secret: true
            toolPanel: selectPanel

            EncryptedSelectPanel {
                id: selectPanel

                active: notesModel.selected > 0

                //: Hint text
                //% "Delete selected notes"
                onDeleteHint: selectPage.showHint(qsTrId("foilnotes-hint-delete_selected"))
                onDeleteSelected: {
                    bulkActionRemorse.cancelNicely()
                    leaveSelectionActive = true
                    thisView.deleteSelectedNotes(notesModel)
                }
                //: Hint text
                //% "Decrypt selected notes"
                onDecryptHint: selectPage.showHint(qsTrId("foilnotes-hint-decrypt_selected"))
                onDecryptSelected: {
                    bulkActionRemorse.cancelNicely()
                    leaveSelectionActive = true
                    thisView.decryptSelectedNotes(notesModel)
                }
            }
        }
    }

    Component {
        id: remorseComponent

        RemorseItem {
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
            wrapMode: Text.Wrap
        }
    }

    NotesGridView {
        id: grid

        columnCount: _columnCount
        page: mainPage
        model: foilModel
        showSelection: bulkActionRemorse.visible
        secret: true

        //: Generic menu item
        //% "Decrypt"
        noteActionMenuText: qsTrId("foilnotes-menu-decrypt")
        onPerformNoteAction: {
            grid.positionViewAtIndex(item.modelIndex, GridView.Visible)
            //: Decrypting note in 5 seconds
            //% "Decrypting"
            item.remorseAction(qsTrId("foilnotes-remorse-decrypting"),
                function(index) { decryptNoteAt(index) })
            pageStack.pop()
        }

        function decryptNoteAt(index) {
            var note = model.get(index)
            if ('body' in note) {
                thisView.decryptNote(note)
                // Always animate deletion of decrypted items
                animateDisplacement = false
                model.deleteNoteAt(index)
                animateDisplacement = !searchMode
                leftSwipeToDecryptedHintLoader.armed = true
            }
        }

        contextMenuComponent: Component {
            ContextMenu {
                id: contextMenu

                // The menu extends across the whole gridview horizontally
                width: thisView.width
                x: parent ? -parent.x : 0

                MenuItem {
                    //: Context menu item
                    //% "Copy to clipboard"
                    text: qsTrId("foilnotes-menu-copy")
                    onClicked: contextMenu.parent.copyToClipboard()
                }

                MenuItem {
                    //: Generic menu item
                    //% "Decrypt"
                    text: qsTrId("foilnotes-menu-decrypt")
                    onClicked: {
                        //: Decrypting note in 5 seconds
                        //% "Decrypting"
                        contextMenu.parent.remorseAction(qsTrId("foilnotes-remorse-decrypting"),
                            function(index) { grid.decryptNoteAt(index) })
                    }
                }

                MenuItem {
                    //: Generic menu item
                    //% "Delete"
                    text: qsTrId("foilnotes-menu-delete")
                    onClicked: contextMenu.parent.deleteNote()
                }
            }
        }

        onCountChanged: pullDownMenu.updateMenuItems()

        PullDownMenu {
            id: pullDownMenu

            enabled: !grid.contextMenuItem

            onActiveChanged: {
                if (!active) {
                    updateMenuItems()
                }
            }

            function updateMenuItems() {
                if (!active) {
                    changePasswordMenuItem.visible = mainPage.isPortrait || !grid.count
                    selectMenuItem.visible = organizeMenuItem.visible = grid.count > 1
                    searchMenuItem.visible = grid.count > 0
                }
            }

            MenuItem {
                id: changePasswordMenuItem

                //: Pulley menu item
                //% "Change password"
                text: qsTrId("foilnotes-menu-change_password")
                visible: mainPage.isPortrait // Reduce number of items in the landscape pulley menu
                onClicked: pageStack.push(Qt.resolvedUrl("foil-ui/FoilUiChangePasswordPage.qml"), {
                    mainPage: thisView.mainPage,
                    foilUi: thisView.foilUi,
                    foilModel: thisView.foilModel,
                    //: Password change prompt
                    //% "Please enter the current and the new password"
                    promptText: qsTrId("foilnotes-change_password_page-label-enter_passwords"),
                    //: Placeholder and label for the current password prompt
                    //% "Current password"
                    currentPasswordLabel: qsTrId("foilnotes-change_password_page-text_field_label-current_password"),
                    //: Placeholder and label for the new password prompt
                    //% "New password"
                    newPasswordLabel: qsTrId("foilnotes-change_password_page-text_field_label-new_password"),
                    //: Button label
                    //% "Change password"
                    buttonText: qsTrId("foilnotes-change_password_page-button-change_password")
                })
            }

            MenuItem {
                id: searchMenuItem

                //: Pulley menu item
                //% "Search"
                text: qsTrId("foilnotes-menu-search")
                onClicked: pageStack.push(Qt.resolvedUrl("SearchPage.qml"), {
                    allowedOrientations: thisPage.allowedOrientations,
                    notesModel: thisView.foilModel,
                    secret: true
                })
            }

            MenuItem {
                id: organizeMenuItem
                //: Pulley menu item
                //% "Organize"
                text: qsTrId("foilnotes-menu-organize")
                onClicked: pageStack.push(Qt.resolvedUrl("OrganizePage.qml"), {
                    //: Page header
                    //% "Organize secret notes"
                    title: qsTrId("foilnotes-organize_page-secret_header"),
                    allowedOrientations: mainPage.allowedOrientations,
                    notesModel: foilModel,
                    secret: true
                })
            }

            MenuItem {
                id: selectMenuItem
                //: Pulley menu item
                //% "Select"
                readonly property string defaultText: qsTrId("foilnotes-menu-select")
                text: defaultText
                onClicked: pageStack.push(selectPageComponent, {
                    notesModel: foilModel,
                    allowedOrientations: mainPage.allowedOrientations
                })
            }

            MenuItem {
                id: newNoteMenuItem
                //: Create a new secret note ready for editing
                //% "New secret note"
                text: qsTrId("foilnotes-menu-new_secret_note")
                onClicked: grid.newNote(foilModel, PageStackAction.Animated)
            }
        }

        RemorsePopup {
            id: bulkActionRemorse

            function cancelNicely() {
                // To avoid flickering, do it only when really necessary
                if (visible) cancel()
            }
        }

        ViewPlaceholder {
            enabled: !thisView.busy && foilModel && foilModel.count === 0
            //: Placeholder text
            //% "You do not have any encrypted notes"
            text: qsTrId("foilnotes-encrypted_view-placeholder")
            //: Placeholder hint
            //% "Open pulley menu to add one."
            hintText: qsTrId("foilnotes-plaintext_view-placeholder_hint")
        }
    }

    Timer {
        id: progressTimer

        interval: 1000
        running: true
    }

    Loader {
        id: leftSwipeToPlaintextHintLoader

        anchors.fill: parent
        // This opacity behavior is just to avoid (fairly harmless but annoying) binding loop for active
        Behavior on opacity { NumberAnimation { duration: 1 } }
        opacity: ((hints.leftSwipeToPlaintext < MaximumHintCount) ||  (item && item.hintRunning)) ? 1 : 0
        active: opacity > 0
        sourceComponent: Component {
            HarbourHorizontalSwipeHint {
                //: Left swipe hint text
                //% "Swipe left to access plaintext notes"
                text: qsTrId("foilnotes-hint-swipe_left_to_plaintext")
                property bool hintCanBeEnabled: mainPage.isCurrentPage &&
                    foilModel.foilState === FoilNotesModel.FoilNotesReady &&
                    hints.leftSwipeToPlaintext < MaximumHintCount
                swipeRight: false
                hintEnabled: hintCanBeEnabled
                onHintShown: hints.leftSwipeToPlaintext++
            }
        }
    }

    Loader {
        id: leftSwipeToDecryptedHintLoader

        property bool armed
        anchors.fill: parent
        // This opacity behavior is just to avoid (fairly harmless but annoying) binding loop for active
        Behavior on opacity { NumberAnimation { duration: 1 } }
        opacity: ((hints.leftSwipeToDecrypted < MaximumHintCount && armed) || (item && item.hintRunning)) ? 1 : 0
        active: opacity > 0
        sourceComponent: Component {
            HarbourHorizontalSwipeHint {
                //: Left swipe hint text
                //% "Decrypted notes are moved back to the right"
                text: qsTrId("foilnotes-hint-swipe_left_to_decrypted")
                swipeRight: false
                hintEnabled: true
                onHintShown: {
                    hints.leftSwipeToDecrypted++
                    leftSwipeToDecryptedHintLoader.armed = false
                }
            }
        }
    }
}
