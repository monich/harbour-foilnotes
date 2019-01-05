import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import harbour.foilnotes 1.0

SilicaGridView {
    id: grid

    anchors.fill: parent
    cellHeight: cellSize
    cellWidth: cellSize
    clip: true

    // Custom note action
    property string noteActionMenuText
    signal performNoteAction(var item)

    property int cellSize
    property int columnCount
    property string filter
    property bool showSelection
    property bool secret
    property bool animateDisplacement: true
    property int removeAnimationDuration: 0
    property Component contextMenuComponent
    property Item contextMenu
    property Item contextMenuItem: contextMenu ? contextMenu.parent : null
    readonly property int cellsPerRow: Math.floor(width/cellWidth)
    readonly property int minOffsetIndex: contextMenuItem ?
        contextMenuItem.modelIndex - (contextMenuItem.modelIndex % columnCount) + columnCount : 0
    readonly property int yOffset: contextMenu ? contextMenu.height : 0

    function newNote() {
        var noteCreated = false
        var notePage = pageStack.push(notePageComponent, {
            color: FoilNotesSettings.pickColor(),
            allowedOrientations: appAllowedOrientations
        })
        notePage.colorChanged.connect(function() {
            if (noteCreated) {
                grid.model.setColorAt(0, notePage.color)
            }
        })
        notePage.saveBody.connect(function(body) {
            if (noteCreated) {
                if (body.length > 0) {
                    model.setBodyAt(0, body)
                } else {
                    grid.model.textIndex = -1
                    grid.model.deleteNoteAt(0)
                    noteCreated = false
                }
            } else if (body.length > 0) {
                model.addNote(notePage.color, body)
                grid.model.textIndex = 0
                noteCreated = true
            }
        })
    }

    function showContextMenu(item) {
        if (!contextMenu) {
            contextMenu = contextMenuComponent.createObject(page)
        }
        // ContextMenu::show is deprecated in Silica 0.25.6 (Dec 2017)
        // and produces an annoying warning
        if ("open" in contextMenu) {
            contextMenu.open(item)
        } else {
            contextMenu.show(item)
        }
    }

    Notification {
        id: clipboardNotification

        //: Pop-up notification
        //% "Copied to clipboard"
        previewBody: qsTrId("foilnotes-notification-copied_to_clipboard")
        expireTimeout: 2000
        Component.onCompleted: {
            if ("icon" in clipboardNotification) {
                clipboardNotification.icon = "icon-s-clipboard"
            }
        }
    }

    Component {
        id: notePageComponent

        NotePage { }
    }

    Component {
        id: remorseComponent

        RemorseItem {
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
            wrapMode: Text.Wrap
        }
    }

    delegate: Component {
        MouseArea {
            id: noteDelegate

            width: grid.cellWidth
            height: menuOpen ? grid.cellHeight + grid.contextMenu.height : grid.cellHeight

            readonly property bool highlighted: (pressed && containsMouse) || menuOpen
            readonly property int modelIndex: model.index
            readonly property string modelText: model.body
            readonly property bool menuOpen: grid.contextMenuItem === noteDelegate

            function deleteNote() {
                grid.positionViewAtIndex(noteDelegate.modelIndex, GridView.Visible)
                //: Remorse item text, will delete note when timer expires
                //% "Deleting"
                remorseAction(qsTrId("foilnotes-remorse-deleting"), function(index) {
                    grid.removeAnimationDuration = 0
                    grid.model.deleteNoteAt(index)
                })
            }

            function remorseAction(text, callback) {
                var item = noteDelegate
                remorseComponent.createObject(item).execute(item, text,
                    function() {
                        var prevAnimateDisplacement  = grid.animateDisplacement
                        grid.animateDisplacement = true
                        callback(item.modelIndex)
                        grid.animateDisplacement = prevAnimateDisplacement
                    })
            }

            function copyToClipboard() {
                Clipboard.text = modelText
                clipboardNotification.publish()
            }

            NoteItem {
                id: noteItem

                filter: grid.filter
                body: modelText
                color: model.color
                pageNumber: model.pagenr
                width: grid.cellWidth
                height: grid.cellHeight
                highlighted: noteDelegate.highlighted
                selected: grid.showSelection && model.selected
                secret: grid.secret

                Connections {
                    target: grid

                    onMinOffsetIndexChanged: updateY()
                    onYOffsetChanged: updateY()
                    function updateY() { noteItem.y = noteDelegate.modelIndex >= grid.minOffsetIndex ? grid.yOffset : 0 }
                }
            }

            onClicked: {
                if (!model.busy) {
                    // Make sure delegate doesn't get destroyed
                    grid.model.textIndex = modelIndex
                    grid.currentIndex = modelIndex
                    var notePage = pageStack.push(notePageComponent, {
                        pagenr: model.pagenr,
                        color: model.color,
                        body: modelText,
                        allowedOrientations: page.allowedOrientations,
                        actionMenuText: noteActionMenuText
                    })
                    notePage.colorChanged.connect(function() { model.color = notePage.color })
                    notePage.saveBody.connect(function(body) { model.body = body })
                    notePage.deleteNote.connect(function() {
                        noteDelegate.deleteNote()
                        pageStack.pop()
                    })
                    notePage.performAction.connect(function() { grid.performNoteAction(noteDelegate) })
                }
            }

            onPressAndHold: {
                if (!noteItem.selected) {
                    grid.showContextMenu(noteDelegate)
                }
            }

            GridView.onAdd: AddAnimation {
                target: noteDelegate
                duration: 150
            }

            GridView.onRemove: SequentialAnimation {
                PropertyAction {
                    target: noteDelegate
                    property: "GridView.delayRemove"
                    value: true
                }
                NumberAnimation {
                    target: noteDelegate
                    properties: "opacity,scale"
                    to: 0
                    duration: grid.removeAnimationDuration
                    easing.type: Easing.InOutQuad
                }
                PropertyAction {
                    target: noteDelegate
                    property: "GridView.delayRemove"
                    value: false
                }
            }
        }
    }

    moveDisplaced: Transition {
        enabled: grid.animateDisplacement
        SmoothedAnimation {
            alwaysRunToEnd: true
            properties: "x,y"
            duration: 150
        }
    }

    removeDisplaced: Transition {
        enabled: grid.animateDisplacement
        SmoothedAnimation {
            alwaysRunToEnd: true
            properties: "x,y"
            duration: 150
        }
    }

    VerticalScrollDecorator { }
}
