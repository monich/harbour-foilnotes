import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property alias notesModel: grid.model
    property Item toolPanel
    property bool secret
    property bool leaveSelectionActive

    readonly property int _columnCount: isPortrait ? appPortraitColumnCount : appLandscapeColumnCount
    readonly property int _cellSize: Math.floor(width / _columnCount)

    Connections {
        target: notesModel
        onSelectedChanged: pullDownMenu.updateMenuItems()
    }

    onStatusChanged: {
        if (!leaveSelectionActive && status === PageStatus.Inactive) {
            notesModel.clearSelection()
        }
    }

    function showHint(text) {
        selectionHint.text = text
        selectionHintTimer.restart()
    }

    SilicaFlickable {
        id: flickable

        clip: true
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: toolPanel.top
        }

        PullDownMenu {
            id: pullDownMenu
            onActiveChanged: updateMenuItems()
            function updateMenuItems() {
                if (!active) {
                    selectNoneMenuItem.visible = notesModel.selected > 0
                    selectAllMenuItem.visible = notesModel.selected < grid.count
                }
            }
            MenuItem {
                id: selectNoneMenuItem
                //: Pulley menu item
                //% "Clear all"
                text: qsTrId("foilnotes-menu-select_none")
                onClicked: notesModel.clearSelection()
            }
            MenuItem {
                id: selectAllMenuItem
                //: Pulley menu item
                //% "Select all"
                text: qsTrId("foilnotes-menu-select_all")
                onClicked: notesModel.selectAll()
            }
        }

        SilicaGridView {
            id: grid

            anchors.fill: parent
            cellHeight: _cellSize
            cellWidth: _cellSize
            clip: true

            readonly property int cellsPerRow: Math.floor(width/cellWidth)

            onCountChanged: pullDownMenu.updateMenuItems()

            //: Page header
            //% "Select notes"
            header: PageHeader { title: qsTrId("foilnotes-select_page-header") }

            delegate: MouseArea {
                id: noteDelegate
                width: grid.cellWidth
                height: grid.cellHeight

                readonly property bool highlighted: pressed && containsMouse
                readonly property int index: model.index

                NoteItem {
                    id: noteItem

                    body: model.body
                    color: model.color
                    pageNumber: model.pagenr
                    anchors.fill: parent
                    highlighted: noteDelegate.highlighted
                    selected: model.selected
                    secret: page.secret
                }

                onClicked: model.selected = !model.selected
            }

            VerticalScrollDecorator { }
        }

        InteractionHintLabel {
            id: selectionHint
            anchors.fill: grid
            visible: opacity > 0
            opacity: selectionHintTimer.running ? 1.0 : 0.0
            Behavior on opacity { FadeAnimation { duration: 1000 } }
        }
    }

    Timer {
        id: selectionHintTimer
        interval: 1000
    }
}
