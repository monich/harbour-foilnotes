import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

import "harbour"

Page {
    id: thisPage

    backNavigation: !grid.contextMenuItem

    property var hints
    property var foilModel
    property var plaintextModel

    property string filter
    property bool searchMode: false

    readonly property int columnCount: isPortrait ? appPortraitColumnCount : appLandscapeColumnCount
    readonly property int cellSize: isPortrait ? appPortraitCellSize : appLandscapeCellSize
    readonly property bool isCurrentPage: status === PageStatus.Active || status === PageStatus.Activating ||
        pageStack.find(function (pageOnStack) { return (thisPage === pageOnStack) })

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
        id: organizePageComponent

        OrganizePage {
            //: Page header
            //% "Organize notes"
            title: qsTrId("foilnotes-organize_page-plaintext_header")
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

    FoilNotesSearchModel {
        id: filterModel

        filterRoleName: "body"
    }

    onIsCurrentPageChanged: {
        if (!isCurrentPage) {
            searchMode = false
        }
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            plaintextModel.textIndex = -1
        }
    }

    onSearchModeChanged: {
        if (searchMode) {
            filter = ""
            filterModel.setFilterFixedString("")
            filterModel.sourceModel = plaintextModel
            grid.model = filterModel
            grid.removeAnimationDuration = 0
            grid.animateDisplacement = false
        } else {
            grid.model = plaintextModel
            grid.animateDisplacement = true
            filterModel.sourceModel = null
            filter = ""
        }
        pullDownMenu.updateMenuItems()
    }

    onFilterChanged: filterModel.setFilterFixedString(filter)

    // Otherwise width is changing with a delay, causing visible layout changes
    onIsLandscapeChanged: width = isLandscape ? Screen.height : Screen.width

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

            enabled: !grid.contextMenuItem

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
                    selectMenuItem.visible = organizeMenuItem.visible = (!thisPage.searchMode) && grid.count > 1
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
                        notesModel: plaintextModel,
                        allowedOrientations: thisPage.allowedOrientations
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
                        notesModel: plaintextModel,
                        allowedOrientations: thisPage.allowedOrientations
                    })
                }
            }

            MenuItem {
                id: newNoteMenuItem

                //: Create a new note ready for editing
                //% "New note"
                text: qsTrId("foilnotes-menu-new_note")
                onClicked: {
                    pullDownMenu.menuItemClicked = true
                    grid.newNote(plaintextModel, PageStackAction.Animated)
                }
            }
        }

        readonly property real searchFieldVisibility: !plaintextModel.count ? 0 : searchMode ? 1 :
            ((pullDownMenu.active && grid.count > 0 && (topMargin > 0) && (contentY + topMargin) < searchField.height) ? (searchField.height - contentY - topMargin) / searchField.height : 0)
        property real searchAreaHeight: searchFieldVisibility * searchField.height

        Behavior on searchAreaHeight { SmoothedAnimation { duration: 200 } }

        SearchField {
            id: searchField

            y: flickable.searchAreaHeight - height
            enabled: searchMode
            width: parent.width
            opacity: flickable.searchFieldVisibility
            visible: opacity > 0

            readonly property bool active: activeFocus || text.length > 0

            onActiveChanged: if (!active) searchMode = false
            onEnabledChanged: {
                text = ""
                if (enabled) {
                    forceActiveFocus()
                }
            }
            onTextChanged: filter = text

            EnterKey.iconSource: "image://theme/icon-m-enter-close"
            EnterKey.onClicked: focus = false

            Behavior on opacity { FadeAnimation {} }
        }

        NotesGridView {
            id: grid

            anchors.topMargin: flickable.searchAreaHeight
            page: thisPage
            columnCount: thisPage.columnCount
            cellSize: thisPage.cellSize
            filter: thisPage.filter
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
        }

        RemorsePopup {
            id: bulkActionRemorse

            function cancelNicely() {
                // To avoid flickering, do it only when really necessary
                if (visible) cancel()
            }
        }

        ViewPlaceholder {
            enabled: !grid.count && !searchMode
            //: Placeholder text
            //% "You do not have any notes."
            text: qsTrId("foilnotes-plaintext_view-placeholder")
            //: Placeholder hint
            //% "Open pulley menu to add one."
            hintText: qsTrId("foilnotes-plaintext_view-placeholder_hint")
        }

        InfoLabel {
            // Something like ViewPlaceholder but without a pulley hint
            opacity: (!grid.count && searchMode) ? 1 : 0
            visible: opacity > 0
            verticalAlignment: Text.AlignVCenter
            anchors {
                top: parent.top
                topMargin: flickable.searchAreaHeight
                bottom: parent.bottom
            }
            //: Placeholder text
            //% "Sorry, couldn't find anything"
            text: qsTrId("foilnotes-search-placeholder")
            Behavior on opacity { FadeAnimation { duration: 150 } }
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
