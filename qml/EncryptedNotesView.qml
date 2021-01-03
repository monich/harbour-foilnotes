import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

import "harbour"

SilicaFlickable {
    id: view

    property Page mainPage
    property var hints
    property var foilUi
    property var foilModel
    property var pulleyFlickable

    property string filter
    property bool searchMode
    readonly property bool busy: foilModel.busy || progressTimer.running

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
                       view.decryptNote(foilModel.get(rows[i]))
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

    Component {
        id: organizePageComponent

        OrganizePage {
            //: Page header
            //% "Organize secret notes"
            title: qsTrId("foilnotes-organize_page-secret_header")
            secret: true
        }
    }

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
                    view.deleteSelectedNotes(notesModel)
                }
                //: Hint text
                //% "Decrypt selected notes"
                onDecryptHint: selectPage.showHint(qsTrId("foilnotes-hint-decrypt_selected"))
                onDecryptSelected: {
                    bulkActionRemorse.cancelNicely()
                    leaveSelectionActive = true
                    view.decryptSelectedNotes(notesModel)
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

    FoilNotesSearchModel {
        id: filterModel

        filterRoleName: "body"
    }

    onSearchModeChanged: {
        if (searchMode) {
            filter = ""
            filterModel.setFilterFixedString("")
            filterModel.sourceModel = foilModel
            grid.model = filterModel
            grid.removeAnimationDuration = 0
            grid.animateDisplacement = false
        } else {
            grid.model = foilModel
            grid.animateDisplacement = true
            filterModel.sourceModel = null
            filter = ""
        }
        pullDownMenu.updateMenuItems()
    }

    onFilterChanged: filterModel.setFilterFixedString(filter)

    SilicaFlickable {
        id: flickable

        anchors.fill: parent

        // atYBeginning property seems to be unreliable, sometimes topMargin
        // equals -contentY but for whatever reason atYBeginning stays false
        readonly property bool fullyExpanded: topMargin > 0 && topMargin == -contentY
        property bool moving

        onMovementStarted: moving = true
        onMovementEnded: moving = false

        PullDownMenu {
            id: pullDownMenu

            property bool searchModeBeforeSnap
            property bool menuItemClicked
            readonly property bool snapped: active && flickable.fullyExpanded && !flickable.moving

            onSnappedChanged: {
                if (snapped) {
                    searchModeBeforeSnap = searchMode
                    if (searchMode) searchField.focus = false
                } else {
                    // Release from snap
                    if (searchModeBeforeSnap) {
                        searchMode = false
                        searchField.text = ""
                    } else if (searchField.opacity > 0) {
                        searchField.focus = true
                        searchMode = true
                    }
                }
            }

            onActiveChanged: {
                if (active) {
                    menuItemClicked = false
                } else {
                    updateMenuItems()
                }
            }

            function updateMenuItems() {
                if (!active) {
                    selectMenuItem.visible = organizeMenuItem.visible = (!view.searchMode) && grid.count > 1
                }
            }

            MenuItem {
                //: Pulley menu item
                //% "Change password"
                text: qsTrId("foilnotes-menu-change_password")
                onClicked: {
                    pullDownMenu.menuItemClicked = true
                    pageStack.push(Qt.resolvedUrl("foil-ui/FoilUiChangePasswordPage.qml"), {
                        mainPage: view.mainPage,
                        foilUi: view.foilUi,
                        foilModel: view.foilModel,
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
            }
            MenuItem {
                id: organizeMenuItem
                //: Pulley menu item
                //% "Organize"
                text: qsTrId("foilnotes-menu-organize")
                onClicked: {
                    pullDownMenu.menuItemClicked = true
                    pageStack.push(organizePageComponent, {
                        notesModel: foilModel,
                        allowedOrientations: mainPage.allowedOrientations
                    })
                }
            }
            MenuItem {
                id: selectMenuItem
                //: Pulley menu item
                //% "Select"
                readonly property string defaultText: qsTrId("foilnotes-menu-select")
                text: defaultText
                onClicked: {
                    pullDownMenu.menuItemClicked = true
                    pageStack.push(selectPageComponent, {
                        notesModel: foilModel,
                        allowedOrientations: mainPage.allowedOrientations
                    })
                }
            }
            MenuItem {
                id: newNoteMenuItem
                //: Create a new secret note ready for editing
                //% "New secret note"
                text: qsTrId("foilnotes-menu-new_secret_note")
                onClicked: {
                    pullDownMenu.menuItemClicked = true
                    grid.newNote(foilModel, PageStackAction.Animated)
                }
            }
        }

        readonly property real searchFieldVisibility: !grid.count ? 0 :  searchMode ? 1 :
            ((pullDownMenu.active && grid.count > 0 && (topMargin > 0) && (contentY + topMargin) < searchField.height) ? (searchField.height - contentY - topMargin) / searchField.height : 0)
        property real searchAreaHeight: searchFieldVisibility * searchField.height

        Behavior on searchAreaHeight { SmoothedAnimation { duration: 200 } }

        SearchField {
            id: searchField

            y: flickable.searchAreaHeight - height
            enabled: searchMode
            width: parent.width - Theme.paddingLarge
            opacity: flickable.searchFieldVisibility
            visible: opacity > 0

            readonly property bool active: activeFocus || text.length > 0

            onActiveChanged: if (!active) searchMode = false
            onEnabledChanged: {
                if (enabled) {
                    text = ""
                    forceActiveFocus()
                }
            }
            onTextChanged: view.filter = text

            EnterKey.iconSource: "image://theme/icon-m-enter-close"
            EnterKey.onClicked: focus = false

            Behavior on opacity { FadeAnimation {} }
        }

        NotesGridView {
            id: grid

            anchors.topMargin: flickable.searchAreaHeight
            columnCount: mainPage.isLandscape ? appLandscapeColumnCount : appPortraitColumnCount
            cellSize: mainPage.isLandscape ? appLandscapeCellSize : appPortraitCellSize
            page: mainPage
            filter: view.filter
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
                    view.decryptNote(note)
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
                    width: view.width
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
        }

        RemorsePopup {
            id: bulkActionRemorse

            function cancelNicely() {
                // To avoid flickering, do it only when really necessary
                if (visible) cancel()
            }
        }

        ViewPlaceholder {
            enabled: !view.busy && foilModel && foilModel.count === 0
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
