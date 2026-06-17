import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

import "harbour"

Page {
    id: thisPage

    property var hints
    property var foilModel
    property var plaintextModel
    readonly property bool isCurrentPage: status === PageStatus.Active || status === PageStatus.Activating ||
        pageStack.find(function (pageOnStack) { return (thisPage === pageOnStack) })

    backNavigation: !grid.contextMenuItem

    readonly property int _columnCount: isPortrait ? appPortraitColumnCount : appLandscapeColumnCount
    readonly property real _landscapeWidth: Screen.height - (('topCutout' in Screen) ? Screen.topCutout.height : 0)

    function openFirstNote(color, body, transition) {
        grid.openFirstNote(color, body, transition)
    }

    function encryptNoteAt(row) {
        if (row >= 0 && foilModel.keyAvailable) {
            grid.removeAnimationDuration = 500
            var reqid = plaintextModel.startEncryptingAt(row)
            var note = plaintextModel.get(row)
            if (!foilModel.encryptNote(note.body, note.color, note.pagenr, reqid)) {
                plaintextModel.onEncryptionDone(reqid, false)
            }
        }
    }

    function newNoteFromCover() {
        grid.newNote(plaintextModel, PageStackAction.Immediate)
    }

    function encryptNotes(rows) {
        pageStack.pop()
        grid.removeAnimationDuration = 500
        for (var i = rows.length - 1; i >= 0 ; i--) {
            encryptNoteAt(rows[i])
        }
    }

    function deleteSelectedNotes(model) {
        var rows = model.selectedRows()
        if (rows.length > 0) {
            pageStack.pop()
            bulkActionRemorse.execute(
                //: Generic remorse popup text
                //% "Deleting %0 selected notes"
                qsTrId("foilnotes-remorse-deleting_selected", rows.length).arg(rows.length),
                function() { plaintextModel.deleteNotes(rows) }
            )
            bulkActionRemorse.canceled.connect(function() {
                model.clearSelection()
            })
        }
    }

    Component {
        id: selectPageComponent

        SelectPage {
            id: selectPage

            toolPanel: selectPanel

            PlaintextSelectPanel {
                id: selectPanel

                active: notesModel.selected > 0
                canEncrypt: foilModel.keyAvailable
                //: Hint text
                //% "Delete selected notes"
                onDeleteHint: selectPage.showHint(qsTrId("foilnotes-hint-delete_selected"))
                onDeleteSelected: {
                    bulkActionRemorse.cancelNicely()
                    leaveSelectionActive = true
                    deleteSelectedNotes(notesModel)
                }
                //: Hint text
                //% "Encrypt selected notes"
                onEncryptHint: selectPage.showHint(qsTrId("foilnotes-hint-encrypt_selected"))
                onEncryptSelected: encryptNotes(notesModel.selectedRows())
            }
        }
    }

    Component.onCompleted: pullDownMenu.updateMenuItems()

    onStatusChanged: {
        if (status === PageStatus.Active) {
            plaintextModel.textIndex = -1
        }
    }

    // Otherwise width is changing with a delay, causing visible layout changes
    // when on-screen keyboard is active and taking part of the screen.
    onIsLandscapeChanged: width = isLandscape ? _landscapeWidth : Screen.width

    NotesGridView {
        id: grid

        page: thisPage
        columnCount: _columnCount
        model: plaintextModel
        showSelection: bulkActionRemorse.visible

        //: Generic menu item
        //% "Encrypt"
        noteActionMenuText: foilModel.keyAvailable ? qsTrId("foilnotes-menu-encrypt") : ""
        onPerformNoteAction: {
            encryptNoteAt(grid.model.sourceRow(item.modelIndex))
            rightSwipeToEncryptedHintLoader.armed = true
            pageStack.pop()
        }

        contextMenuComponent: Component {
            ContextMenu {
                id: contextMenu

                // The menu extends across the whole gridview horizontally
                width: thisPage.width
                x: parent ? -parent.x : 0

                MenuItem {
                    //: Context menu item
                    //% "Copy to clipboard"
                    text: qsTrId("foilnotes-menu-copy")
                    onClicked: contextMenu.parent.copyToClipboard()
                }
                MenuItem {
                    //: Generic menu item
                    //% "Encrypt"
                    text: qsTrId("foilnotes-menu-encrypt")
                    visible: foilModel.keyAvailable
                    onClicked: {
                        encryptNoteAt(grid.model.sourceRow(contextMenu.parent.modelIndex))
                        rightSwipeToEncryptedHintLoader.armed = true
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
                    selectMenuItem.visible = organizeMenuItem.visible = grid.count > 1
                    searchMenuItem.visible = grid.count > 0
                }
            }

            MenuItem {
                id: searchMenuItem

                //: Pulley menu item
                //% "Search"
                text: qsTrId("foilnotes-menu-search")
                onClicked: pageStack.push(Qt.resolvedUrl("SearchPage.qml"), {
                    notesModel: plaintextModel,
                    allowedOrientations: thisPage.allowedOrientations
                })
            }

            MenuItem {
                id: organizeMenuItem

                //: Pulley menu item
                //% "Organize"
                text: qsTrId("foilnotes-menu-organize")
                onClicked: pageStack.push(Qt.resolvedUrl("OrganizePage.qml"), {
                    //: Page header
                    //% "Organize notes"
                    title: qsTrId("foilnotes-organize_page-plaintext_header"),
                    notesModel: plaintextModel,
                    allowedOrientations: thisPage.allowedOrientations
                })
            }

            MenuItem {
                id: selectMenuItem

                //: Pulley menu item
                //% "Select"
                readonly property string defaultText: qsTrId("foilnotes-menu-select")
                text: defaultText
                onClicked: pageStack.push(selectPageComponent, {
                    notesModel: plaintextModel,
                    allowedOrientations: thisPage.allowedOrientations
                })
            }

            MenuItem {
                id: newNoteMenuItem

                //: Create a new note ready for editing
                //% "New note"
                text: qsTrId("foilnotes-menu-new_note")
                onClicked: grid.newNote(plaintextModel, PageStackAction.Animated)
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
            enabled: !plaintextModel.count
            //: Placeholder text
            //% "You do not have any notes."
            text: qsTrId("foilnotes-plaintext_view-placeholder")
            //: Placeholder hint
            //% "Open pulley menu to add one."
            hintText: qsTrId("foilnotes-plaintext_view-placeholder_hint")
        }
    }

    Loader {
        id: rightSwipeToEncryptedHintLoader

        property bool armed
        anchors.fill: parent
        // This opacity behavior is just to avoid (fairly harmless but annoying) binding loop for active
        Behavior on opacity { NumberAnimation { duration: 1 } }
        opacity: ((hints.rightSwipeToEncrypted < MaximumHintCount && armed) || (item && item.hintRunning)) ? 1 : 0
        active: opacity > 0
        sourceComponent: Component {
            HarbourHorizontalSwipeHint {
                //: Right swipe hint text
                //% "Encrypted pictures are moved there to the left"
                text: qsTrId("foilnotes-hint-swipe_right_to_encrypted")
                swipeRight: true
                hintEnabled: true
                onHintShown: {
                    hints.rightSwipeToEncrypted++
                    rightSwipeToEncryptedHintLoader.armed = false
                }
            }
        }
    }
}
