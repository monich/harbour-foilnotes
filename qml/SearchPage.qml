import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

Page {
    id: thisPage

    property alias notesModel: filterModel.sourceModel
    property alias secret: grid.secret

    readonly property int _columnCount: isPortrait ? appPortraitColumnCount : appLandscapeColumnCount

    backNavigation: !grid.contextMenuItem

    Component.onCompleted: grid.headerItem.forceActiveFocus()

    NotesGridView {
        id: grid

        page: thisPage
        columnCount: _columnCount
        filter: headerItem.text

        model: FoilNotesSearchModel {
            id: filterModel

            filterRoleName: "body"
        }

        header: SearchField {
            id: searchField

            width: parent.width
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase

            onHideClicked: {
                requestFocusTimer.stop()
                keepFocusTimer.stop()
                searchActive = false
            }
            onTextChanged: {
                focusOutBehavior = FocusBehavior.KeepFocus
                filterModel.setFilterFixedString(text) // steals the focus
                requestFocusTimer.start()
            }

            EnterKey.iconSource: "image://theme/icon-m-enter-close"
            EnterKey.onClicked: focus = false

            Timer {
                id: requestFocusTimer

                interval: 1
                onTriggered: {
                    searchField.focus = true
                    searchField.forceActiveFocus()
                    keepFocusTimer.start()
                }
            }

            Timer {
                id: keepFocusTimer

                interval: 100
                onTriggered: searchField.focusOutBehavior = FocusBehavior.ClearPageFocus
            }
        }

        contextMenuComponent: ContextMenu {
            id: contextMenu

            // The menu extends across the whole gridview horizontally
            width: grid.width
            x: parent ? -parent.x : 0

            MenuItem {
                //: Context menu item
                //% "Copy to clipboard"
                text: qsTrId("foilnotes-menu-copy")
                onClicked: contextMenu.parent.copyToClipboard()
            }
            MenuItem {
                //: Generic menu item
                //% "Delete"
                text: qsTrId("foilnotes-menu-delete")
                onClicked: contextMenu.parent.deleteNote()
            }
        }

        InfoLabel {
            // Something like ViewPlaceholder but without a pulley hint
            opacity: grid.count ? 0 : 1
            visible: opacity > 0
            verticalAlignment: Text.AlignVCenter
            anchors {
                top: parent.top
                topMargin: grid.headerItem.height
                bottom: parent.bottom
            }
            //: Placeholder text
            //% "Sorry, couldn't find anything"
            text: qsTrId("foilnotes-search-placeholder")
            Behavior on opacity { FadeAnimation { duration: 150 } }
        }
    }
}
