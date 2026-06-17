import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.foilnotes 1.0

SilicaGridView {
    id: thisView

    anchors.fill: parent
    cellHeight: _cellSize
    cellWidth: _cellSize
    currentIndex: -1
    clip: true

    // Custom note action
    property string noteActionMenuText
    signal performNoteAction(var item)

    property Page page
    property int columnCount
    property string filter
    property bool showSelection
    property bool secret
    property bool animateDisplacement: true
    property int removeAnimationDuration: 0
    property Component contextMenuComponent
    readonly property Item contextMenuItem: _contextMenu ? _contextMenu.parent : null

    property Item _contextMenu
    readonly property int _expandHeight: _contextMenu ? _contextMenu.height : 0
    readonly property int _cellSize: Math.floor(width / columnCount)
    readonly property int _minOffsetIndex: contextMenuItem ?
        contextMenuItem.modelIndex - (contextMenuItem.modelIndex % columnCount) + columnCount : 0

    function openFirstNote(color,body,transition) {
        _openNote(function() {return 0}, function() {return 1},color,body,transition)
    }

    function _openNote(indexFn,pagenrFn,color,body,transition) {
        model.textIndex = indexFn()
        currentIndex = model.textIndex
        var notePage = pageStack.push(notePageComponent, {
            pagenr: pagenrFn(),
            color: color,
            body: body,
            secret: secret,
            allowedOrientations: allowedOrientations,
            actionMenuText: noteActionMenuText
        }, transition)
        notePage.pagenr = Qt.binding(pagenrFn)
        notePage.colorChanged.connect(function() { model.setColorAt(indexFn(), notePage.color) })
        notePage.saveBody.connect(function(body) { model.setBodyAt(indexFn(),body) })
        notePage.deleteNote.connect(function() {
            currentItem.deleteNote()
            pageStack.pop()
        })
        notePage.performAction.connect(function() { thisView.performNoteAction(currentItem) })
    }

    function newNote(model,transition) {
        var noteCreated = false
        var notePage = pageStack.push(notePageComponent, {
            color: FoilNotesSettings.pickColor(),
            allowedOrientations: page.allowedOrientations
        }, transition)
        notePage.colorChanged.connect(function() {
            if (noteCreated) {
                model.setColorAt(0, notePage.color)
            }
        })
        notePage.saveBody.connect(function(body) {
            if (noteCreated) {
                if (body.length > 0) {
                    model.setBodyAt(0, body)
                    model.setColorAt(0, notePage.color)
                } else {
                    model.deleteNoteAt(0)
                    noteCreated = false
                }
            } else if (body.length > 0) {
                model.addNote(notePage.color, body)
                model.textIndex = 0
                currentIndex = 0
                noteCreated = true
            }
        })
        notePage.deleteNote.connect(function() {
            if (noteCreated) {
                if (notePage.body.length > 0) {
                    model.setBodyAt(0, notePage.body)
                    model.setColorAt(0, notePage.color)
                    thisView.positionViewAtBeginning()
                    thisView.currentItem.deleteNote()
                } else {
                    model.deleteNoteAt(0)
                }
            } else if (notePage.body.length > 0) {
                model.addNote(notePage.color, notePage.body)
                currentIndex = 0
                thisView.positionViewAtBeginning()
                thisView.currentItem.deleteNote()
            }
            // Reset the dirty flag to prevent save on pop
            notePage.dirty = false
            pageStack.pop()
        })
    }

    function showContextMenu(item) {
        if (!_contextMenu) {
            _contextMenu = contextMenuComponent.createObject(page)
        }
        focus = false // close the vkb
        // ContextMenu::show is deprecated in Silica 0.25.6 (Dec 2017)
        // and produces an annoying warning
        if ("open" in _contextMenu) {
            _contextMenu.open(item)
        } else {
            _contextMenu.show(item)
        }
    }

    onMovementStarted: focus = false // close the vkb

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

            width: thisView.cellWidth
            height: menuOpen ? thisView.cellHeight + thisView._contextMenu.height : thisView.cellHeight
            enabled: !thisView.contextMenuItem

            readonly property bool down: pressed && containsMouse
            readonly property bool selected: thisView.showSelection && model.selected
            readonly property bool highlighted: down || menuOpen || selected
            readonly property int modelIndex: model.index
            readonly property int modelPageNr: model.pagenr
            readonly property string modelText: model.body
            readonly property bool menuOpen: thisView.contextMenuItem === noteDelegate
            readonly property real contentYOffset: index >= thisView._minOffsetIndex ? thisView._expandHeight : 0.0

            function deleteNote() {
                thisView.positionViewAtIndex(noteDelegate.modelIndex, GridView.Visible)
                //: Remorse item text, will delete note when timer expires
                //% "Deleting"
                remorseAction(qsTrId("foilnotes-remorse-deleting"), function(index) {
                    thisView.removeAnimationDuration = 0
                    thisView.model.deleteNoteAt(index)
                })
            }

            function remorseAction(text, callback) {
                var delegateItem = noteDelegate
                var remorseParent = noteItem
                remorseComponent.createObject(remorseParent).execute(remorseParent, text,
                    function() {
                        var prevAnimateDisplacement  = thisView.animateDisplacement
                        thisView.animateDisplacement = true
                        callback(delegateItem.modelIndex)
                        thisView.animateDisplacement = prevAnimateDisplacement
                    })
            }

            function copyToClipboard() {
                Clipboard.text = modelText
            }

            NoteItem {
                id: noteItem

                y: noteDelegate.contentYOffset
                filter: thisView.filter
                body: modelText
                color: model.color
                pageNumber: model.pagenr
                width: thisView.cellWidth
                height: thisView.cellHeight
                pressed: noteDelegate.down && !noteDelegate.menuOpen
                selected: noteDelegate.selected
                highlighted: noteDelegate.highlighted
                secret: thisView.secret
            }

            onClicked: {
                if (!model.busy) {
                    _openNote(function() { return modelIndex }, function() { return modelPageNr },
                        model.color, modelText, PageStackAction.Animated)
                }
            }

            onPressAndHold: {
                if (!noteItem.selected) {
                    thisView.showContextMenu(noteDelegate)
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
                    duration: thisView.removeAnimationDuration
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
        enabled: thisView.animateDisplacement
        SmoothedAnimation {
            alwaysRunToEnd: true
            properties: "x,y"
            duration: 150
        }
    }

    removeDisplaced: Transition {
        enabled: thisView.animateDisplacement
        SmoothedAnimation {
            alwaysRunToEnd: true
            properties: "x,y"
            duration: 150
        }
    }

    VerticalScrollDecorator { flickable: thisView }
}
